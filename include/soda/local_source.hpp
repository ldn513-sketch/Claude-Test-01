/*
 * SODA Player - Local File Source
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_LOCAL_SOURCE_HPP
#define SODA_LOCAL_SOURCE_HPP

#include "soda.hpp"
#include "source_manager.hpp"

namespace soda {

class EventBus;
class MetadataReader;

class LocalSource : public Source {
public:
    explicit LocalSource(EventBus& eventBus);
    ~LocalSource() override;

    // Source interface
    SourceType type() const override { return SourceType::Local; }
    std::string name() const override { return "Local Files"; }
    bool isAvailable() const override { return true; }

    std::vector<SearchResult> search(const std::string& query) override;
    Result<TrackInfo> getTrack(const std::string& id) override;
    Result<std::string> getStreamUrl(const std::string& id) override;
    Result<Path> download(const std::string& id, const Path& destination) override;

    // Local-specific functionality
    void addFolder(const Path& folder);
    void removeFolder(const Path& folder);
    std::vector<Path> watchedFolders() const;

    // Scanning
    void scan();
    void scanFolder(const Path& folder);
    void rescanAll();
    bool isScanning() const { return scanning_.load(); }

    // File watching
    void startWatching();
    void stopWatching();

    // Library access
    std::vector<TrackInfo> allTracks() const;
    std::vector<std::string> allArtists() const;
    std::vector<std::string> allAlbums() const;
    std::vector<std::string> allGenres() const;

    std::vector<TrackInfo> tracksByArtist(const std::string& artist) const;
    std::vector<TrackInfo> tracksByAlbum(const std::string& album) const;
    std::vector<TrackInfo> tracksByGenre(const std::string& genre) const;
    std::vector<TrackInfo> tracksByYear(int year) const;
    std::vector<TrackInfo> tracksByDecade(const std::string& decade) const; // "1990s", etc.

    // Recent and favorites
    std::vector<TrackInfo> recentlyAdded(size_t limit = 50) const;
    std::vector<TrackInfo> recentlyPlayed(size_t limit = 50) const;
    std::vector<TrackInfo> mostPlayed(size_t limit = 50) const;

    // Statistics
    size_t totalTracks() const;
    size_t totalArtists() const;
    size_t totalAlbums() const;
    Duration totalDuration() const;

private:
    void indexFile(const Path& file);
    void removeFile(const Path& file);
    std::string generateId(const Path& file);
    bool isSupportedFormat(const Path& file) const;

    EventBus& eventBus_;
    std::unique_ptr<MetadataReader> metadataReader_;

    std::vector<Path> watchedFolders_;
    std::unordered_map<std::string, TrackInfo> tracks_;
    std::unordered_map<std::string, std::vector<std::string>> artistTracks_;
    std::unordered_map<std::string, std::vector<std::string>> albumTracks_;
    std::unordered_map<std::string, std::vector<std::string>> genreTracks_;

    std::atomic<bool> scanning_{false};
    std::thread scanThread_;
    std::thread watchThread_;
    std::atomic<bool> watching_{false};

    mutable std::mutex mutex_;

    static const std::vector<std::string> SUPPORTED_EXTENSIONS;
};

} // namespace soda

#endif // SODA_LOCAL_SOURCE_HPP
