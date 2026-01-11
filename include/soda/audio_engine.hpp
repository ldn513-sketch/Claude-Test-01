/*
 * SODA Player - Audio Engine
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_AUDIO_ENGINE_HPP
#define SODA_AUDIO_ENGINE_HPP

#include "soda.hpp"
#include "queue.hpp"

namespace soda {

class EventBus;
class AudioDecoder;

class AudioEngine {
public:
    explicit AudioEngine(EventBus& eventBus);
    ~AudioEngine();

    // Initialization
    Result<void> initialize();
    void shutdown();

    // Playback control
    Result<void> play();
    Result<void> pause();
    Result<void> stop();
    Result<void> togglePlayPause();

    // Navigation
    Result<void> playTrack(const TrackInfo& track);
    Result<void> playNext();
    Result<void> playPrevious();
    Result<void> seek(Duration position);

    // Queue management
    Queue& queue() { return queue_; }
    const Queue& queue() const { return queue_; }

    // State
    PlaybackState state() const { return state_.load(); }
    Duration position() const;
    Duration duration() const;
    std::optional<TrackInfo> currentTrack() const;

    // Volume (0.0 - 1.0)
    void setVolume(float volume);
    float volume() const { return volume_.load(); }

    // Repeat and shuffle
    void setRepeatMode(RepeatMode mode) { repeatMode_ = mode; }
    RepeatMode repeatMode() const { return repeatMode_; }
    void setShuffle(bool enabled) { shuffle_ = enabled; }
    bool shuffle() const { return shuffle_; }

    // Streaming
    Result<void> playUrl(const std::string& url);

    // Audio data callback (for visualizers via plugins)
    using AudioDataCallback = std::function<void(const float* data, size_t frameCount, int channels)>;
    void setAudioDataCallback(AudioDataCallback callback);

private:
    void audioCallback(void* output, size_t frameCount);
    void updateProgress();
    void onTrackEnded();
    void startProgressTimer();
    void stopProgressTimer();

    EventBus& eventBus_;
    Queue queue_;

    std::unique_ptr<AudioDecoder> decoder_;
    void* device_ = nullptr; // ma_device pointer
    void* context_ = nullptr;

    std::atomic<PlaybackState> state_{PlaybackState::Stopped};
    std::atomic<float> volume_{0.8f};
    std::atomic<uint64_t> currentFrame_{0};
    std::atomic<uint64_t> totalFrames_{0};

    RepeatMode repeatMode_ = RepeatMode::Off;
    bool shuffle_ = false;

    std::optional<TrackInfo> currentTrack_;
    std::mutex trackMutex_;

    AudioDataCallback audioDataCallback_;
    std::mutex callbackMutex_;

    // Progress timer
    std::thread progressThread_;
    std::atomic<bool> progressRunning_{false};
    std::condition_variable progressCv_;
    std::mutex progressMutex_;

    bool initialized_ = false;
};

} // namespace soda

#endif // SODA_AUDIO_ENGINE_HPP
