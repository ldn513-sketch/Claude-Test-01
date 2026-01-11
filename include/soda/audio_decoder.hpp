/*
 * SODA Player - Audio Decoder
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_AUDIO_DECODER_HPP
#define SODA_AUDIO_DECODER_HPP

#include "soda.hpp"

namespace soda {

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    // Open a file or URL
    Result<void> open(const Path& filePath);
    Result<void> openUrl(const std::string& url);
    void close();

    // Decoding
    size_t readFrames(float* output, size_t frameCount);
    bool seek(uint64_t frameIndex);

    // Properties
    bool isOpen() const { return isOpen_; }
    bool isStreaming() const { return isStreaming_; }
    uint64_t totalFrames() const { return totalFrames_; }
    uint64_t currentFrame() const { return currentFrame_; }
    uint32_t sampleRate() const { return sampleRate_; }
    uint32_t channels() const { return channels_; }
    Duration duration() const;
    Duration position() const;

    // Format detection
    static AudioFormat detectFormat(const Path& filePath);
    static std::string formatToString(AudioFormat format);
    static AudioFormat stringToFormat(const std::string& str);

private:
    Result<void> openWithMiniaudio(const Path& filePath);
    Result<void> openWithFfmpeg(const Path& filePath);
    Result<void> setupStreamingDecoder(const std::string& url);

    void* decoder_ = nullptr; // ma_decoder pointer
    bool isOpen_ = false;
    bool isStreaming_ = false;

    uint64_t totalFrames_ = 0;
    uint64_t currentFrame_ = 0;
    uint32_t sampleRate_ = 44100;
    uint32_t channels_ = 2;

    // Streaming buffer
    std::vector<float> streamBuffer_;
    std::mutex streamMutex_;
    std::condition_variable streamCv_;
    std::thread streamThread_;
    std::atomic<bool> streamRunning_{false};

    // Resampling (if needed)
    void* resampler_ = nullptr;
    uint32_t targetSampleRate_ = 44100;
};

} // namespace soda

#endif // SODA_AUDIO_DECODER_HPP
