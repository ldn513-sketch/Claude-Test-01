/*
 * SODA Player - Audio Decoder Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/audio_decoder.hpp>
#include "miniaudio.h"
#include <cstring>
#include <algorithm>

namespace soda {

AudioDecoder::AudioDecoder() = default;

AudioDecoder::~AudioDecoder() {
    close();
}

Result<void> AudioDecoder::open(const Path& filePath) {
    close();

    if (!std::filesystem::exists(filePath)) {
        return Result<void>("File not found: " + filePath.string());
    }

    return openWithMiniaudio(filePath);
}

Result<void> AudioDecoder::openWithMiniaudio(const Path& filePath) {
    decoder_ = new ma_decoder();

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 44100);

    if (ma_decoder_init_file(filePath.c_str(), &config, static_cast<ma_decoder*>(decoder_)) != MA_SUCCESS) {
        delete static_cast<ma_decoder*>(decoder_);
        decoder_ = nullptr;
        return Result<void>("Failed to open audio file");
    }

    ma_decoder* dec = static_cast<ma_decoder*>(decoder_);
    sampleRate_ = dec->outputSampleRate;
    channels_ = dec->outputChannels;

    ma_decoder_get_length_in_pcm_frames(dec, &totalFrames_);
    currentFrame_ = 0;
    isOpen_ = true;
    isStreaming_ = false;

    return Result<void>();
}

Result<void> AudioDecoder::openUrl(const std::string& url) {
    // For URL streaming, we'd need to implement HTTP streaming
    // This is a placeholder for now
    return Result<void>("URL streaming not yet implemented");
}

void AudioDecoder::close() {
    if (streamRunning_.load()) {
        streamRunning_.store(false);
        streamCv_.notify_all();
        if (streamThread_.joinable()) {
            streamThread_.join();
        }
    }

    if (decoder_) {
        ma_decoder_uninit(static_cast<ma_decoder*>(decoder_));
        delete static_cast<ma_decoder*>(decoder_);
        decoder_ = nullptr;
    }

    isOpen_ = false;
    isStreaming_ = false;
    totalFrames_ = 0;
    currentFrame_ = 0;
}

size_t AudioDecoder::readFrames(float* output, size_t frameCount) {
    if (!isOpen_ || !decoder_) {
        return 0;
    }

    uint64_t framesRead = 0;
    ma_decoder_read_pcm_frames(static_cast<ma_decoder*>(decoder_), output, frameCount, &framesRead);
    currentFrame_ += framesRead;

    return static_cast<size_t>(framesRead);
}

bool AudioDecoder::seek(uint64_t frameIndex) {
    if (!isOpen_ || !decoder_) {
        return false;
    }

    if (ma_decoder_seek_to_pcm_frame(static_cast<ma_decoder*>(decoder_), frameIndex) == MA_SUCCESS) {
        currentFrame_ = frameIndex;
        return true;
    }
    return false;
}

Duration AudioDecoder::duration() const {
    if (sampleRate_ == 0) {
        return Duration(0);
    }
    return Duration((totalFrames_ * 1000) / sampleRate_);
}

Duration AudioDecoder::position() const {
    if (sampleRate_ == 0) {
        return Duration(0);
    }
    return Duration((currentFrame_ * 1000) / sampleRate_);
}

AudioFormat AudioDecoder::detectFormat(const Path& filePath) {
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".mp3") return AudioFormat::MP3;
    if (ext == ".m4a" || ext == ".aac") return AudioFormat::M4A;
    if (ext == ".flac") return AudioFormat::FLAC;
    if (ext == ".ogg" || ext == ".oga") return AudioFormat::OGG;
    if (ext == ".opus") return AudioFormat::OPUS;
    if (ext == ".wav" || ext == ".wave") return AudioFormat::WAV;

    return AudioFormat::Unknown;
}

std::string AudioDecoder::formatToString(AudioFormat format) {
    switch (format) {
        case AudioFormat::MP3: return "MP3";
        case AudioFormat::M4A: return "M4A";
        case AudioFormat::FLAC: return "FLAC";
        case AudioFormat::OGG: return "OGG";
        case AudioFormat::OPUS: return "OPUS";
        case AudioFormat::WAV: return "WAV";
        default: return "Unknown";
    }
}

AudioFormat AudioDecoder::stringToFormat(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "mp3") return AudioFormat::MP3;
    if (lower == "m4a" || lower == "aac") return AudioFormat::M4A;
    if (lower == "flac") return AudioFormat::FLAC;
    if (lower == "ogg" || lower == "oga") return AudioFormat::OGG;
    if (lower == "opus") return AudioFormat::OPUS;
    if (lower == "wav" || lower == "wave") return AudioFormat::WAV;

    return AudioFormat::Unknown;
}

} // namespace soda
