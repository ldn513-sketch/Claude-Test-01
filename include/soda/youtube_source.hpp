/*
 * SODA Player - YouTube Source
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 *
 * Note: Downloading content from YouTube may violate their Terms of Service.
 * Users are responsible for ensuring their use complies with applicable laws
 * and terms of service.
 */

#ifndef SODA_YOUTUBE_SOURCE_HPP
#define SODA_YOUTUBE_SOURCE_HPP

#include "soda.hpp"
#include "source_manager.hpp"

namespace soda {

class EventBus;
class HttpClient;

// YouTube video/audio format info
struct YouTubeFormat {
    std::string itag;
    std::string mimeType;
    std::string quality;
    std::string audioQuality;
    int bitrate = 0;
    int sampleRate = 0;
    int channels = 0;
    std::string url;
    bool hasAudio = false;
    bool hasVideo = false;
    int64_t contentLength = 0;
};

// YouTube video info
struct YouTubeVideoInfo {
    std::string videoId;
    std::string title;
    std::string author;
    std::string channelId;
    int lengthSeconds = 0;
    std::string thumbnail;
    std::vector<YouTubeFormat> formats;
    std::vector<YouTubeFormat> adaptiveFormats;

    // Chapter/timestamp info for full albums
    struct Chapter {
        std::string title;
        int startTimeSeconds = 0;
        int endTimeSeconds = 0;
    };
    std::vector<Chapter> chapters;
};

// YouTube playlist info
struct YouTubePlaylistInfo {
    std::string playlistId;
    std::string title;
    std::string author;
    std::string description;
    std::string thumbnail;
    int videoCount = 0;
    std::vector<std::string> videoIds;
};

class YouTubeSource : public Source {
public:
    explicit YouTubeSource(EventBus& eventBus);
    ~YouTubeSource() override;

    // Source interface
    SourceType type() const override { return SourceType::YouTube; }
    std::string name() const override { return "YouTube"; }
    bool isAvailable() const override;

    std::vector<SearchResult> search(const std::string& query) override;
    Result<TrackInfo> getTrack(const std::string& id) override;
    Result<std::string> getStreamUrl(const std::string& id) override;
    Result<Path> download(const std::string& id, const Path& destination) override;

    // YouTube-specific functionality

    // Video info
    Result<YouTubeVideoInfo> getVideoInfo(const std::string& videoId);

    // Audio extraction
    struct AudioOptions {
        std::string preferredQuality = "high"; // "high", "medium", "low"
        std::string preferredFormat = "opus";   // "opus", "m4a"
    };
    Result<std::string> getAudioUrl(const std::string& videoId, const AudioOptions& options = {});

    // Playlists
    Result<YouTubePlaylistInfo> getPlaylistInfo(const std::string& playlistId);
    Result<std::vector<std::string>> getPlaylistVideoIds(const std::string& playlistId);

    // Import playlist as local playlist
    Result<PlaylistInfo> importPlaylist(const std::string& playlistUrl);

    // Full album handling
    bool hasChapters(const std::string& videoId);
    std::vector<TrackInfo> splitByChapters(const std::string& videoId);

    // Download with progress
    using ProgressCallback = std::function<void(int64_t downloaded, int64_t total)>;
    Result<Path> downloadAudio(const std::string& videoId,
                               const Path& destination,
                               const AudioOptions& options = {},
                               ProgressCallback progress = nullptr);

    // URL parsing
    static std::optional<std::string> parseVideoId(const std::string& url);
    static std::optional<std::string> parsePlaylistId(const std::string& url);
    static bool isYouTubeUrl(const std::string& url);

    // Cache management
    void clearCache();
    size_t cacheSize() const;

private:
    Result<YouTubeVideoInfo> fetchVideoInfo(const std::string& videoId);
    Result<std::string> decipherSignature(const std::string& signature, const std::string& playerUrl);
    std::optional<YouTubeFormat> selectBestAudioFormat(const std::vector<YouTubeFormat>& formats,
                                                        const AudioOptions& options);
    TrackInfo videoInfoToTrack(const YouTubeVideoInfo& info);

    EventBus& eventBus_;
    std::unique_ptr<HttpClient> httpClient_;

    // Cache for video info (to avoid repeated API calls)
    std::unordered_map<std::string, YouTubeVideoInfo> infoCache_;
    mutable std::mutex cacheMutex_;
};

} // namespace soda

#endif // SODA_YOUTUBE_SOURCE_HPP
