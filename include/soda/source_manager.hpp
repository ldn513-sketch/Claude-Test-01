/*
 * SODA Player - Source Manager
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_SOURCE_MANAGER_HPP
#define SODA_SOURCE_MANAGER_HPP

#include "soda.hpp"

namespace soda {

class LocalSource;
class YouTubeSource;
class PodcastSource;
class EventBus;
class ConfigManager;

// Abstract base for all sources
class Source {
public:
    virtual ~Source() = default;

    virtual SourceType type() const = 0;
    virtual std::string name() const = 0;
    virtual bool isAvailable() const = 0;

    // Search
    virtual std::vector<SearchResult> search(const std::string& query) = 0;

    // Track operations
    virtual Result<TrackInfo> getTrack(const std::string& id) = 0;
    virtual Result<std::string> getStreamUrl(const std::string& id) = 0;

    // Download
    virtual Result<Path> download(const std::string& id, const Path& destination) = 0;
};

class SourceManager {
public:
    SourceManager(EventBus& eventBus, ConfigManager& config);
    ~SourceManager();

    // Initialize all sources
    Result<void> initialize(const Path& dataDir);
    void shutdown();

    // Source access
    LocalSource& local() { return *localSource_; }
    YouTubeSource& youtube() { return *youtubeSource_; }
    PodcastSource& podcasts() { return *podcastSource_; }

    // Unified search across all sources
    std::vector<SearchResult> search(const std::string& query,
                                     bool includeLocal = true,
                                     bool includeYouTube = true,
                                     bool includePodcasts = true);

    // Get track from any source
    Result<TrackInfo> getTrack(SourceType type, const std::string& id);
    Result<std::string> getStreamUrl(SourceType type, const std::string& id);

    // Download from any source
    Result<Path> download(SourceType type, const std::string& id);

    // Library database
    void rebuildDatabase();
    size_t trackCount() const;
    std::vector<TrackInfo> getAllTracks() const;
    std::vector<std::string> getAllArtists() const;
    std::vector<std::string> getAllAlbums() const;
    std::vector<TrackInfo> getTracksByArtist(const std::string& artist) const;
    std::vector<TrackInfo> getTracksByAlbum(const std::string& album) const;

private:
    void loadDatabase();
    void saveDatabase();

    EventBus& eventBus_;
    ConfigManager& config_;
    Path dataDir_;

    std::unique_ptr<LocalSource> localSource_;
    std::unique_ptr<YouTubeSource> youtubeSource_;
    std::unique_ptr<PodcastSource> podcastSource_;

    // In-memory track database
    std::unordered_map<std::string, TrackInfo> trackDatabase_;
    mutable std::mutex dbMutex_;
};

} // namespace soda

#endif // SODA_SOURCE_MANAGER_HPP
