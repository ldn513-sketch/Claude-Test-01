/*
 * SODA Player - Podcast Source
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_PODCAST_SOURCE_HPP
#define SODA_PODCAST_SOURCE_HPP

#include "soda.hpp"
#include "source_manager.hpp"

namespace soda {

class EventBus;
class HttpClient;

class PodcastSource : public Source {
public:
    explicit PodcastSource(EventBus& eventBus);
    ~PodcastSource() override;

    // Source interface
    SourceType type() const override { return SourceType::Podcast; }
    std::string name() const override { return "Podcasts"; }
    bool isAvailable() const override;

    std::vector<SearchResult> search(const std::string& query) override;
    Result<TrackInfo> getTrack(const std::string& id) override;
    Result<std::string> getStreamUrl(const std::string& id) override;
    Result<Path> download(const std::string& id, const Path& destination) override;

    // Podcast-specific functionality

    // Feed management
    Result<PodcastFeed> subscribe(const std::string& feedUrl);
    void unsubscribe(const std::string& feedId);
    std::vector<PodcastFeed> subscriptions() const;

    // Feed info
    Result<PodcastFeed> getFeedInfo(const std::string& feedUrl);
    Result<PodcastFeed> refreshFeed(const std::string& feedId);
    void refreshAllFeeds();

    // Episodes
    std::vector<PodcastEpisode> getEpisodes(const std::string& feedId) const;
    std::vector<PodcastEpisode> getNewEpisodes() const;
    std::vector<PodcastEpisode> getDownloadedEpisodes() const;
    Result<PodcastEpisode> getEpisode(const std::string& episodeId) const;

    // Playback tracking
    void markAsPlayed(const std::string& episodeId);
    void markAsUnplayed(const std::string& episodeId);
    void savePlaybackPosition(const std::string& episodeId, Duration position);
    Duration getPlaybackPosition(const std::string& episodeId) const;

    // Download episodes
    using ProgressCallback = std::function<void(int64_t downloaded, int64_t total)>;
    Result<Path> downloadEpisode(const std::string& episodeId,
                                  const Path& destination,
                                  ProgressCallback progress = nullptr);

    // Catalog search (iTunes, Podcast Index)
    struct CatalogResult {
        std::string feedUrl;
        std::string title;
        std::string author;
        std::string description;
        std::string artworkUrl;
        std::vector<std::string> categories;
    };
    std::vector<CatalogResult> searchCatalog(const std::string& query);

    // Import/Export OPML
    Result<void> importOpml(const Path& opmlFile);
    Result<void> exportOpml(const Path& opmlFile) const;

    // Persistence
    Result<void> save(const Path& dataDir);
    Result<void> load(const Path& dataDir);

private:
    Result<PodcastFeed> parseFeed(const std::string& feedUrl, const std::string& xmlContent);
    std::vector<PodcastEpisode> parseEpisodes(const std::string& feedId, const std::string& xmlContent);
    std::string generateEpisodeId(const std::string& feedId, const std::string& guid);
    bool isAudioEnclosure(const std::string& mimeType) const;

    EventBus& eventBus_;
    std::unique_ptr<HttpClient> httpClient_;

    std::unordered_map<std::string, PodcastFeed> feeds_;
    std::unordered_map<std::string, std::vector<PodcastEpisode>> episodes_;
    std::unordered_map<std::string, Duration> playbackPositions_;

    mutable std::mutex mutex_;
};

} // namespace soda

#endif // SODA_PODCAST_SOURCE_HPP
