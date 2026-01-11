/*
 * SODA Player - Podcast Source Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/podcast_source.hpp>
#include <soda/http_client.hpp>
#include <soda/event_bus.hpp>
#include <soda/string_utils.hpp>
#include <yaml-cpp/yaml.h>
#include <regex>
#include <fstream>
#include <sstream>

namespace soda {

PodcastSource::PodcastSource(EventBus& eventBus)
    : eventBus_(eventBus)
    , httpClient_(std::make_unique<HttpClient>())
{
}

PodcastSource::~PodcastSource() = default;

bool PodcastSource::isAvailable() const {
    return true; // Podcasts are always available (RSS is HTTP-based)
}

std::vector<SearchResult> PodcastSource::search(const std::string& query) {
    std::vector<SearchResult> results;

    // Search in subscribed feeds
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& [feedId, feed] : feeds_) {
        if (string_utils::containsIgnoreCase(feed.title, query) ||
            string_utils::containsIgnoreCase(feed.author, query)) {

            SearchResult result;
            result.id = "podcast:" + feedId;
            result.title = feed.title;
            result.subtitle = feed.author;
            result.thumbnailUrl = feed.imageUrl;
            result.source = SourceType::Podcast;
            result.sourceId = feedId;
            result.isPlaylist = true;

            results.push_back(result);
        }
    }

    // Also search episodes
    for (const auto& [feedId, episodeList] : episodes_) {
        for (const auto& episode : episodeList) {
            if (string_utils::containsIgnoreCase(episode.title, query)) {
                SearchResult result;
                result.id = "podcast:" + episode.id;
                result.title = episode.title;

                auto feedIt = feeds_.find(feedId);
                if (feedIt != feeds_.end()) {
                    result.subtitle = feedIt->second.title;
                    result.thumbnailUrl = feedIt->second.imageUrl;
                }

                result.source = SourceType::Podcast;
                result.sourceId = episode.id;
                result.duration = episode.duration;
                result.isPlaylist = false;

                results.push_back(result);
            }
        }
    }

    return results;
}

Result<TrackInfo> PodcastSource::getTrack(const std::string& id) {
    std::string episodeId = id;
    if (episodeId.rfind("podcast:", 0) == 0) {
        episodeId = episodeId.substr(8);
    }

    auto episodeResult = getEpisode(episodeId);
    if (!episodeResult.ok()) {
        return Result<TrackInfo>(episodeResult.error());
    }

    const auto& episode = episodeResult.value();

    TrackInfo track;
    track.id = "podcast:" + episode.id;
    track.title = episode.title;
    track.duration = episode.duration;
    track.source = SourceType::Podcast;
    track.sourceId = episode.id;

    // Get feed info for artist/album
    std::lock_guard<std::mutex> lock(mutex_);
    auto feedIt = feeds_.find(episode.feedId);
    if (feedIt != feeds_.end()) {
        track.artist = feedIt->second.author;
        track.album = feedIt->second.title;
        track.coverUrl = feedIt->second.imageUrl;
    }

    return Result<TrackInfo>(track);
}

Result<std::string> PodcastSource::getStreamUrl(const std::string& id) {
    std::string episodeId = id;
    if (episodeId.rfind("podcast:", 0) == 0) {
        episodeId = episodeId.substr(8);
    }

    auto episodeResult = getEpisode(episodeId);
    if (!episodeResult.ok()) {
        return Result<std::string>(episodeResult.error());
    }

    return Result<std::string>(episodeResult.value().audioUrl);
}

Result<Path> PodcastSource::download(const std::string& id, const Path& destination) {
    std::string episodeId = id;
    if (episodeId.rfind("podcast:", 0) == 0) {
        episodeId = episodeId.substr(8);
    }

    return downloadEpisode(episodeId, destination);
}

Result<PodcastFeed> PodcastSource::subscribe(const std::string& feedUrl) {
    auto feedResult = getFeedInfo(feedUrl);
    if (!feedResult.ok()) {
        return feedResult;
    }

    PodcastFeed feed = feedResult.value();

    std::lock_guard<std::mutex> lock(mutex_);
    feeds_[feed.id] = feed;

    return Result<PodcastFeed>(feed);
}

void PodcastSource::unsubscribe(const std::string& feedId) {
    std::lock_guard<std::mutex> lock(mutex_);
    feeds_.erase(feedId);
    episodes_.erase(feedId);
}

std::vector<PodcastFeed> PodcastSource::subscriptions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PodcastFeed> result;
    result.reserve(feeds_.size());
    for (const auto& [id, feed] : feeds_) {
        result.push_back(feed);
    }
    return result;
}

Result<PodcastFeed> PodcastSource::getFeedInfo(const std::string& feedUrl) {
    auto response = httpClient_->get(feedUrl);
    if (!response.success()) {
        return Result<PodcastFeed>("Failed to fetch feed: " + response.error);
    }

    return parseFeed(feedUrl, response.body);
}

Result<PodcastFeed> PodcastSource::refreshFeed(const std::string& feedId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = feeds_.find(feedId);
    if (it == feeds_.end()) {
        return Result<PodcastFeed>("Feed not found");
    }

    std::string feedUrl = it->second.feedUrl;
    lock.~lock_guard();

    return getFeedInfo(feedUrl);
}

void PodcastSource::refreshAllFeeds() {
    std::vector<std::string> feedIds;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [id, feed] : feeds_) {
            feedIds.push_back(id);
        }
    }

    for (const auto& feedId : feedIds) {
        refreshFeed(feedId);
    }
}

std::vector<PodcastEpisode> PodcastSource::getEpisodes(const std::string& feedId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = episodes_.find(feedId);
    if (it != episodes_.end()) {
        return it->second;
    }
    return {};
}

std::vector<PodcastEpisode> PodcastSource::getNewEpisodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PodcastEpisode> result;

    for (const auto& [feedId, episodeList] : episodes_) {
        for (const auto& episode : episodeList) {
            if (!episode.isPlayed) {
                result.push_back(episode);
            }
        }
    }

    // Sort by publish date (newest first)
    std::sort(result.begin(), result.end(),
              [](const PodcastEpisode& a, const PodcastEpisode& b) {
                  return a.publishedAt > b.publishedAt;
              });

    return result;
}

std::vector<PodcastEpisode> PodcastSource::getDownloadedEpisodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PodcastEpisode> result;

    for (const auto& [feedId, episodeList] : episodes_) {
        for (const auto& episode : episodeList) {
            if (episode.downloadedPath.has_value()) {
                result.push_back(episode);
            }
        }
    }

    return result;
}

Result<PodcastEpisode> PodcastSource::getEpisode(const std::string& episodeId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& [feedId, episodeList] : episodes_) {
        for (const auto& episode : episodeList) {
            if (episode.id == episodeId) {
                return Result<PodcastEpisode>(episode);
            }
        }
    }

    return Result<PodcastEpisode>("Episode not found: " + episodeId);
}

void PodcastSource::markAsPlayed(const std::string& episodeId) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [feedId, episodeList] : episodes_) {
        for (auto& episode : episodeList) {
            if (episode.id == episodeId) {
                episode.isPlayed = true;
                return;
            }
        }
    }
}

void PodcastSource::markAsUnplayed(const std::string& episodeId) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [feedId, episodeList] : episodes_) {
        for (auto& episode : episodeList) {
            if (episode.id == episodeId) {
                episode.isPlayed = false;
                return;
            }
        }
    }
}

void PodcastSource::savePlaybackPosition(const std::string& episodeId, Duration position) {
    std::lock_guard<std::mutex> lock(mutex_);
    playbackPositions_[episodeId] = position;

    for (auto& [feedId, episodeList] : episodes_) {
        for (auto& episode : episodeList) {
            if (episode.id == episodeId) {
                episode.playbackPosition = position;
                return;
            }
        }
    }
}

Duration PodcastSource::getPlaybackPosition(const std::string& episodeId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = playbackPositions_.find(episodeId);
    if (it != playbackPositions_.end()) {
        return it->second;
    }
    return Duration(0);
}

Result<Path> PodcastSource::downloadEpisode(const std::string& episodeId,
                                             const Path& destination,
                                             ProgressCallback progress) {
    auto episodeResult = getEpisode(episodeId);
    if (!episodeResult.ok()) {
        return Result<Path>(episodeResult.error());
    }

    const auto& episode = episodeResult.value();
    std::string filename = string_utils::slugify(episode.title) + ".mp3";
    Path outputPath = destination / filename;

    auto downloadResult = httpClient_->downloadFile(episode.audioUrl, outputPath, progress);
    if (!downloadResult.ok()) {
        return Result<Path>(downloadResult.error());
    }

    // Update episode with download path
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [feedId, episodeList] : episodes_) {
            for (auto& ep : episodeList) {
                if (ep.id == episodeId) {
                    ep.downloadedPath = outputPath;
                    break;
                }
            }
        }
    }

    return Result<Path>(outputPath);
}

std::vector<PodcastSource::CatalogResult> PodcastSource::searchCatalog(const std::string& query) {
    std::vector<CatalogResult> results;

    // Search iTunes Podcast API
    std::string url = "https://itunes.apple.com/search?media=podcast&term=" +
                      HttpClient::urlEncode(query);

    auto response = httpClient_->get(url);
    if (!response.success()) {
        return results;
    }

    // Parse JSON response (simplified - use proper JSON parser in production)
    std::regex feedRegex(R"("feedUrl":"([^"]+)")");
    std::regex titleRegex(R"("trackName":"([^"]+)")");
    std::regex authorRegex(R"("artistName":"([^"]+)")");
    std::regex artworkRegex(R"("artworkUrl600":"([^"]+)")");

    // This is a very simplified parser - use proper JSON in production
    std::string::const_iterator searchStart = response.body.cbegin();
    std::smatch feedMatch;

    while (std::regex_search(searchStart, response.body.cend(), feedMatch, feedRegex)) {
        CatalogResult result;
        result.feedUrl = feedMatch[1].str();

        // Would need to properly parse JSON for other fields
        results.push_back(result);

        searchStart = feedMatch.suffix().first;
        if (results.size() >= 20) break;
    }

    return results;
}

Result<void> PodcastSource::importOpml(const Path& opmlFile) {
    std::ifstream file(opmlFile);
    if (!file) {
        return Result<void>("Failed to open OPML file");
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    std::regex outlineRegex(R"(<outline[^>]+xmlUrl="([^"]+)"[^>]*>)");

    auto begin = std::sregex_iterator(content.begin(), content.end(), outlineRegex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::string feedUrl = (*it)[1].str();
        subscribe(feedUrl); // Ignore errors, continue with other feeds
    }

    return Result<void>();
}

Result<void> PodcastSource::exportOpml(const Path& opmlFile) const {
    std::ofstream file(opmlFile);
    if (!file) {
        return Result<void>("Failed to open file for writing");
    }

    file << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n";
    file << R"(<opml version="1.0">)" << "\n";
    file << R"(  <head>)" << "\n";
    file << R"(    <title>SODA Player Podcast Subscriptions</title>)" << "\n";
    file << R"(  </head>)" << "\n";
    file << R"(  <body>)" << "\n";

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, feed] : feeds_) {
        file << R"(    <outline type="rss" text=")" << string_utils::escapeXml(feed.title)
             << R"(" xmlUrl=")" << string_utils::escapeXml(feed.feedUrl) << R"("/>)" << "\n";
    }

    file << R"(  </body>)" << "\n";
    file << R"(</opml>)" << "\n";

    return Result<void>();
}

Result<void> PodcastSource::save(const Path& dataDir) {
    Path podcastDir = dataDir / "podcasts";
    std::filesystem::create_directories(podcastDir);

    YAML::Emitter out;
    out << YAML::BeginMap;

    out << YAML::Key << "feeds" << YAML::Value << YAML::BeginSeq;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [id, feed] : feeds_) {
            out << YAML::BeginMap;
            out << YAML::Key << "id" << YAML::Value << feed.id;
            out << YAML::Key << "title" << YAML::Value << feed.title;
            out << YAML::Key << "author" << YAML::Value << feed.author;
            out << YAML::Key << "feedUrl" << YAML::Value << feed.feedUrl;
            out << YAML::Key << "imageUrl" << YAML::Value << feed.imageUrl;
            out << YAML::EndMap;
        }
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;

    std::ofstream file(podcastDir / "subscriptions.yaml");
    if (!file) {
        return Result<void>("Failed to save podcast subscriptions");
    }
    file << out.c_str();

    return Result<void>();
}

Result<void> PodcastSource::load(const Path& dataDir) {
    Path subFile = dataDir / "podcasts" / "subscriptions.yaml";
    if (!std::filesystem::exists(subFile)) {
        return Result<void>();
    }

    try {
        YAML::Node node = YAML::LoadFile(subFile.string());

        std::lock_guard<std::mutex> lock(mutex_);
        feeds_.clear();

        if (node["feeds"]) {
            for (const auto& feedNode : node["feeds"]) {
                PodcastFeed feed;
                feed.id = feedNode["id"].as<std::string>();
                feed.title = feedNode["title"].as<std::string>("");
                feed.author = feedNode["author"].as<std::string>("");
                feed.feedUrl = feedNode["feedUrl"].as<std::string>();
                feed.imageUrl = feedNode["imageUrl"].as<std::string>("");
                feeds_[feed.id] = feed;
            }
        }

        return Result<void>();

    } catch (const std::exception& e) {
        return Result<void>("Failed to load podcasts: " + std::string(e.what()));
    }
}

Result<PodcastFeed> PodcastSource::parseFeed(const std::string& feedUrl, const std::string& xmlContent) {
    PodcastFeed feed;
    feed.feedUrl = feedUrl;
    feed.id = string_utils::md5(feedUrl);

    // Simple XML parsing with regex (use proper XML parser in production)
    std::regex titleRegex(R"(<title>([^<]+)</title>)");
    std::regex authorRegex(R"(<itunes:author>([^<]+)</itunes:author>)");
    std::regex descRegex(R"(<description>([^<]+)</description>)");
    std::regex imageRegex(R"(<itunes:image[^>]+href="([^"]+)")");

    std::smatch match;

    if (std::regex_search(xmlContent, match, titleRegex)) {
        feed.title = match[1].str();
    }
    if (std::regex_search(xmlContent, match, authorRegex)) {
        feed.author = match[1].str();
    }
    if (std::regex_search(xmlContent, match, descRegex)) {
        feed.description = match[1].str();
    }
    if (std::regex_search(xmlContent, match, imageRegex)) {
        feed.imageUrl = match[1].str();
    }

    // Parse episodes
    auto episodeList = parseEpisodes(feed.id, xmlContent);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        episodes_[feed.id] = episodeList;
    }

    feed.lastUpdated = Clock::now();
    return Result<PodcastFeed>(feed);
}

std::vector<PodcastEpisode> PodcastSource::parseEpisodes(const std::string& feedId,
                                                          const std::string& xmlContent) {
    std::vector<PodcastEpisode> result;

    // Find all <item> elements
    std::regex itemRegex(R"(<item>([\s\S]*?)</item>)");
    std::regex titleRegex(R"(<title>([^<]+)</title>)");
    std::regex guidRegex(R"(<guid[^>]*>([^<]+)</guid>)");
    std::regex enclosureRegex(R"(<enclosure[^>]+url="([^"]+)"[^>]+type="([^"]+)")");
    std::regex durationRegex(R"(<itunes:duration>([^<]+)</itunes:duration>)");

    auto itemBegin = std::sregex_iterator(xmlContent.begin(), xmlContent.end(), itemRegex);
    auto itemEnd = std::sregex_iterator();

    for (auto it = itemBegin; it != itemEnd; ++it) {
        std::string itemContent = (*it)[1].str();

        PodcastEpisode episode;
        episode.feedId = feedId;

        std::smatch match;

        if (std::regex_search(itemContent, match, titleRegex)) {
            episode.title = match[1].str();
        }
        if (std::regex_search(itemContent, match, guidRegex)) {
            episode.id = generateEpisodeId(feedId, match[1].str());
        }
        if (std::regex_search(itemContent, match, enclosureRegex)) {
            std::string mimeType = match[2].str();
            if (isAudioEnclosure(mimeType)) {
                episode.audioUrl = match[1].str();
            }
        }
        if (std::regex_search(itemContent, match, durationRegex)) {
            episode.duration = string_utils::parseDuration(match[1].str());
        }

        if (!episode.audioUrl.empty()) {
            result.push_back(episode);
        }
    }

    return result;
}

std::string PodcastSource::generateEpisodeId(const std::string& feedId, const std::string& guid) {
    return string_utils::md5(feedId + ":" + guid);
}

bool PodcastSource::isAudioEnclosure(const std::string& mimeType) const {
    return mimeType.find("audio/") == 0;
}

} // namespace soda
