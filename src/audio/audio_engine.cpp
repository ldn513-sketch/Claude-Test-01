/*
 * SODA Player - Audio Engine Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <soda/audio_engine.hpp>
#include <soda/audio_decoder.hpp>
#include <soda/event_bus.hpp>
#include <cstring>
#include <algorithm>

namespace soda {

// Static callback wrapper
static void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    AudioEngine* engine = static_cast<AudioEngine*>(pDevice->pUserData);
    if (engine) {
        engine->audioCallback(pOutput, frameCount);
    }
}

AudioEngine::AudioEngine(EventBus& eventBus)
    : eventBus_(eventBus)
    , decoder_(std::make_unique<AudioDecoder>())
{
}

AudioEngine::~AudioEngine() {
    shutdown();
}

Result<void> AudioEngine::initialize() {
    if (initialized_) {
        return Result<void>();
    }

    // Allocate device
    device_ = new ma_device();

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.sampleRate = 44100;
    config.channels = 2;
    config.format = ma_format_f32;
    config.dataCallback = reinterpret_cast<void(*)(void*, void*, const void*, uint32_t)>(audioDataCallback);
    config.pUserData = this;

    if (ma_device_init(nullptr, &config, static_cast<ma_device*>(device_)) != MA_SUCCESS) {
        delete static_cast<ma_device*>(device_);
        device_ = nullptr;
        return Result<void>("Failed to initialize audio device");
    }

    initialized_ = true;
    return Result<void>();
}

void AudioEngine::shutdown() {
    stop();
    stopProgressTimer();

    if (device_) {
        ma_device_uninit(static_cast<ma_device*>(device_));
        delete static_cast<ma_device*>(device_);
        device_ = nullptr;
    }

    decoder_->close();
    initialized_ = false;
}

void AudioEngine::audioCallback(void* output, size_t frameCount) {
    float* out = static_cast<float*>(output);

    if (state_.load() != PlaybackState::Playing || !decoder_->isOpen()) {
        std::memset(out, 0, frameCount * 2 * sizeof(float));
        return;
    }

    size_t framesRead = decoder_->readFrames(out, frameCount);
    currentFrame_.fetch_add(framesRead);

    // Apply volume
    float vol = volume_.load();
    for (size_t i = 0; i < framesRead * 2; ++i) {
        out[i] *= vol;
    }

    // Zero remaining frames if we didn't get enough
    if (framesRead < frameCount) {
        std::memset(out + framesRead * 2, 0, (frameCount - framesRead) * 2 * sizeof(float));
    }

    // Notify audio data callback (for visualizers)
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (audioDataCallback_) {
            audioDataCallback_(out, framesRead, 2);
        }
    }

    // Check if track ended
    if (decoder_->currentFrame() >= decoder_->totalFrames()) {
        onTrackEnded();
    }
}

Result<void> AudioEngine::play() {
    if (!initialized_) {
        return Result<void>("Audio engine not initialized");
    }

    if (state_.load() == PlaybackState::Playing) {
        return Result<void>();
    }

    // If stopped, try to play first track in queue
    if (state_.load() == PlaybackState::Stopped) {
        auto track = queue_.current();
        if (track) {
            auto result = playTrack(*track);
            if (!result.ok()) {
                return result;
            }
        } else {
            return Result<void>("Queue is empty");
        }
    }

    // Start device if paused
    if (ma_device_start(static_cast<ma_device*>(device_)) != MA_SUCCESS) {
        return Result<void>("Failed to start audio device");
    }

    state_.store(PlaybackState::Playing);
    startProgressTimer();

    {
        std::lock_guard<std::mutex> lock(trackMutex_);
        if (currentTrack_) {
            eventBus_.emitPlaybackStarted(*currentTrack_);
        }
    }

    return Result<void>();
}

Result<void> AudioEngine::pause() {
    if (state_.load() != PlaybackState::Playing) {
        return Result<void>();
    }

    ma_device_stop(static_cast<ma_device*>(device_));
    state_.store(PlaybackState::Paused);
    stopProgressTimer();

    eventBus_.emitPlaybackPaused();
    return Result<void>();
}

Result<void> AudioEngine::stop() {
    if (state_.load() == PlaybackState::Stopped) {
        return Result<void>();
    }

    ma_device_stop(static_cast<ma_device*>(device_));
    decoder_->close();
    state_.store(PlaybackState::Stopped);
    currentFrame_.store(0);
    totalFrames_.store(0);
    stopProgressTimer();

    {
        std::lock_guard<std::mutex> lock(trackMutex_);
        currentTrack_.reset();
    }

    eventBus_.emitPlaybackStopped();
    return Result<void>();
}

Result<void> AudioEngine::togglePlayPause() {
    if (state_.load() == PlaybackState::Playing) {
        return pause();
    } else {
        return play();
    }
}

Result<void> AudioEngine::playTrack(const TrackInfo& track) {
    // Stop current playback
    ma_device_stop(static_cast<ma_device*>(device_));
    decoder_->close();

    // Open new track
    Result<void> result = decoder_->open(track.filePath);
    if (!result.ok()) {
        // Try stream URL if local file fails
        if (!track.sourceId.empty()) {
            result = decoder_->openUrl(track.sourceId);
        }
        if (!result.ok()) {
            return result;
        }
    }

    totalFrames_.store(decoder_->totalFrames());
    currentFrame_.store(0);

    {
        std::lock_guard<std::mutex> lock(trackMutex_);
        currentTrack_ = track;
    }

    // Start playback
    if (ma_device_start(static_cast<ma_device*>(device_)) != MA_SUCCESS) {
        return Result<void>("Failed to start audio device");
    }

    state_.store(PlaybackState::Playing);
    startProgressTimer();

    eventBus_.emitTrackChanged(track);
    eventBus_.emitPlaybackStarted(track);

    return Result<void>();
}

Result<void> AudioEngine::playNext() {
    auto track = queue_.next();
    if (track) {
        return playTrack(*track);
    }

    // No more tracks, stop
    stop();
    return Result<void>();
}

Result<void> AudioEngine::playPrevious() {
    auto track = queue_.previous();
    if (track) {
        return playTrack(*track);
    }
    return Result<void>();
}

Result<void> AudioEngine::seek(Duration position) {
    if (!decoder_->isOpen()) {
        return Result<void>("No track loaded");
    }

    uint64_t frame = (position.count() * decoder_->sampleRate()) / 1000;
    if (decoder_->seek(frame)) {
        currentFrame_.store(frame);
        return Result<void>();
    }
    return Result<void>("Seek failed");
}

Duration AudioEngine::position() const {
    if (decoder_->sampleRate() == 0) {
        return Duration(0);
    }
    return Duration((currentFrame_.load() * 1000) / decoder_->sampleRate());
}

Duration AudioEngine::duration() const {
    if (decoder_->sampleRate() == 0) {
        return Duration(0);
    }
    return Duration((totalFrames_.load() * 1000) / decoder_->sampleRate());
}

std::optional<TrackInfo> AudioEngine::currentTrack() const {
    std::lock_guard<std::mutex> lock(trackMutex_);
    return currentTrack_;
}

void AudioEngine::setVolume(float volume) {
    volume_.store(std::clamp(volume, 0.0f, 1.0f));
    eventBus_.emitVolumeChanged(volume_.load());
}

Result<void> AudioEngine::playUrl(const std::string& url) {
    ma_device_stop(static_cast<ma_device*>(device_));
    decoder_->close();

    auto result = decoder_->openUrl(url);
    if (!result.ok()) {
        return result;
    }

    totalFrames_.store(decoder_->totalFrames());
    currentFrame_.store(0);

    if (ma_device_start(static_cast<ma_device*>(device_)) != MA_SUCCESS) {
        return Result<void>("Failed to start audio device");
    }

    state_.store(PlaybackState::Playing);
    startProgressTimer();

    return Result<void>();
}

void AudioEngine::setAudioDataCallback(AudioDataCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    audioDataCallback_ = std::move(callback);
}

void AudioEngine::onTrackEnded() {
    // Check repeat mode
    if (repeatMode_ == RepeatMode::One) {
        decoder_->seek(0);
        currentFrame_.store(0);
        return;
    }

    // Try to play next
    auto track = queue_.next();
    if (track) {
        // Queue playTrack on main thread
        eventBus_.publishAsync(Event{EventType::TrackChanged, *track});
    } else if (repeatMode_ == RepeatMode::All && !queue_.isEmpty()) {
        // Loop back to start
        queue_.jumpTo(0);
        track = queue_.current();
        if (track) {
            eventBus_.publishAsync(Event{EventType::TrackChanged, *track});
        }
    } else {
        // Stop playback
        state_.store(PlaybackState::Stopped);
        eventBus_.publishAsync(Event{EventType::PlaybackStopped});
    }
}

void AudioEngine::startProgressTimer() {
    if (progressRunning_.load()) {
        return;
    }

    progressRunning_.store(true);
    progressThread_ = std::thread([this]() {
        while (progressRunning_.load()) {
            {
                std::unique_lock<std::mutex> lock(progressMutex_);
                progressCv_.wait_for(lock, std::chrono::milliseconds(250));
            }

            if (progressRunning_.load() && state_.load() == PlaybackState::Playing) {
                eventBus_.emitPlaybackProgress(position(), duration());
            }
        }
    });
}

void AudioEngine::stopProgressTimer() {
    if (!progressRunning_.load()) {
        return;
    }

    progressRunning_.store(false);
    progressCv_.notify_all();

    if (progressThread_.joinable()) {
        progressThread_.join();
    }
}

} // namespace soda
