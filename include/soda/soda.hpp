/*
 * SODA Player - Main Header
 *
 * This file is part of SODA Player.
 * Copyright (C) 2026 SODA Project Contributors
 *
 * SODA Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SODA Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SODA Player. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SODA_HPP
#define SODA_HPP

#define SODA_VERSION_MAJOR 0
#define SODA_VERSION_MINOR 1
#define SODA_VERSION_PATCH 0
#define SODA_VERSION_STRING "0.1.0"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <filesystem>
#include <chrono>
#include <variant>
#include <map>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <condition_variable>

namespace soda {

// Forward declarations
class Application;
class AudioEngine;
class AudioDecoder;
class Playlist;
class Queue;
class ConfigManager;
class Settings;
class PluginManager;
class PluginInterface;
class SourceManager;
class LocalSource;
class YouTubeSource;
class PodcastSource;
class SkinManager;
class WebViewWindow;
class JsBridge;
class HttpClient;
class MetadataReader;

// Type aliases
using Path = std::filesystem::path;
using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = std::chrono::milliseconds;

// Common enums
enum class PlaybackState {
    Stopped,
    Playing,
    Paused,
    Buffering
};

enum class RepeatMode {
    Off,
    One,
    All
};

enum class AudioFormat {
    Unknown,
    MP3,
    M4A,
    FLAC,
    OGG,
    OPUS,
    WAV
};

enum class SourceType {
    Local,
    YouTube,
    Podcast
};

// Track information
struct TrackInfo {
    std::string id;
    std::string title;
    std::string artist;
    std::string album;
    std::string genre;
    int year = 0;
    int trackNumber = 0;
    Duration duration{0};
    Path filePath;
    std::string coverUrl;
    SourceType source = SourceType::Local;
    std::string sourceId; // YouTube video ID, podcast episode ID, etc.
    AudioFormat format = AudioFormat::Unknown;
    int bitrate = 0;
    int sampleRate = 44100;
    int channels = 2;

    // Metadata for dynamic queue/radio mode
    std::string country;
    std::string era; // Decade like "1990s"
    std::vector<std::string> tags;

    bool isDownloaded = false;
    std::optional<Path> cachedPath;
};

// Playlist information
struct PlaylistInfo {
    std::string id;
    std::string name;
    std::string description;
    std::vector<std::string> trackIds;
    TimePoint createdAt;
    TimePoint modifiedAt;
    std::optional<std::string> coverPath;
    SourceType source = SourceType::Local;
    std::string sourceId; // For YouTube playlists, podcast feeds, etc.
};

// Podcast specific
struct PodcastFeed {
    std::string id;
    std::string title;
    std::string author;
    std::string description;
    std::string feedUrl;
    std::string imageUrl;
    std::string website;
    std::vector<std::string> categories;
    TimePoint lastUpdated;
};

struct PodcastEpisode {
    std::string id;
    std::string feedId;
    std::string title;
    std::string description;
    std::string audioUrl;
    Duration duration{0};
    TimePoint publishedAt;
    std::optional<Path> downloadedPath;
    bool isPlayed = false;
    Duration playbackPosition{0};
};

// Plugin related
struct PluginInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    Path path;
    std::vector<std::string> permissions;
    bool isEnabled = false;
};

// Event system
enum class EventType {
    PlaybackStarted,
    PlaybackPaused,
    PlaybackStopped,
    PlaybackProgress,
    TrackChanged,
    QueueChanged,
    PlaylistChanged,
    VolumeChanged,
    ConfigChanged,
    PluginLoaded,
    PluginUnloaded,
    SourceUpdated,
    DownloadStarted,
    DownloadProgress,
    DownloadCompleted,
    DownloadFailed,
    Error
};

struct Event {
    EventType type;
    std::variant<
        std::monostate,
        TrackInfo,
        PlaylistInfo,
        std::string,
        int,
        double,
        bool
    > data;
    TimePoint timestamp = Clock::now();
};

using EventCallback = std::function<void(const Event&)>;

// Search results
struct SearchResult {
    std::string id;
    std::string title;
    std::string subtitle;
    std::string thumbnailUrl;
    SourceType source;
    std::string sourceId;
    Duration duration{0};
    bool isPlaylist = false;
};

// Error handling
class SodaException : public std::exception {
public:
    explicit SodaException(std::string message) : message_(std::move(message)) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

// Result type for operations that can fail
template<typename T>
class Result {
public:
    Result(T value) : value_(std::move(value)), hasValue_(true) {}
    Result(std::string error) : error_(std::move(error)), hasValue_(false) {}

    bool ok() const { return hasValue_; }
    const T& value() const {
        if (!hasValue_) throw SodaException(error_);
        return value_;
    }
    T& value() {
        if (!hasValue_) throw SodaException(error_);
        return value_;
    }
    const std::string& error() const { return error_; }

    operator bool() const { return hasValue_; }

private:
    T value_{};
    std::string error_;
    bool hasValue_;
};

template<>
class Result<void> {
public:
    Result() : hasValue_(true) {}
    Result(std::string error) : error_(std::move(error)), hasValue_(false) {}

    bool ok() const { return hasValue_; }
    const std::string& error() const { return error_; }

    operator bool() const { return hasValue_; }

private:
    std::string error_;
    bool hasValue_;
};

} // namespace soda

#endif // SODA_HPP
