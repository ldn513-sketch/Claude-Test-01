/*
 * SODA Player - Source Manager Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/source_manager.hpp>
#include <soda/local_source.hpp>
#include <soda/youtube_source.hpp>
#include <soda/podcast_source.hpp>
#include <soda/event_bus.hpp>
#include <soda/config_manager.hpp>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <algorithm>

namespace soda {

SourceManager::SourceManager(EventBus& eventBus, ConfigManager& config)
    : eventBus_(eventBus)
    , config_(config)
{
}

SourceManager::~SourceManager() {
    shutdown();
}

Result<void> SourceManager::initialize(const Path& dataDir) {
    dataDir_ = dataDir;

    // Initialize sources
    localSource_ = std::make_unique<LocalSource>(eventBus_);
    youtubeSource_ = std::make_unique<YouTubeSource>(eventBus_);
    podcastSource_ = std::make_unique<PodcastSource>(eventBus_);

    // Add configured music folders
    for (const auto& folder : config_.settings().musicFolders) {
        localSource_->addFolder(folder);
    }

    // Initial scan
    localSource_->scan();

    // Load database
    loadDatabase();

    // Load podcast subscriptions
    podcastSource_->load(dataDir);

    return Result<void>();
}

void SourceManager::shutdown() {
    saveDatabase();

    if (podcastSource_) {
        podcastSource_->save(dataDir_);
    }

    if (localSource_) {
        localSource_->stopWatching();
    }
}

std::vector<SearchResult> SourceManager::search(const std::string& query,
                                                  bool includeLocal,
                                                  bool includeYouTube,
                                                  bool includePodcasts) {
    std::vector<SearchResult> results;

    if (includeLocal && localSource_) {
        auto localResults = localSource_->search(query);
        results.insert(results.end(), localResults.begin(), localResults.end());
    }

    if (includeYouTube && youtubeSource_ && config_.settings().youtubeEnabled) {
        auto ytResults = youtubeSource_->search(query);
        results.insert(results.end(), ytResults.begin(), ytResults.end());
    }

    if (includePodcasts && podcastSource_ && config_.settings().podcastsEnabled) {
        auto podResults = podcastSource_->search(query);
        results.insert(results.end(), podResults.begin(), podResults.end());
    }

    return results;
}

Result<TrackInfo> SourceManager::getTrack(SourceType type, const std::string& id) {
    switch (type) {
        case SourceType::Local:
            return localSource_->getTrack(id);
        case SourceType::YouTube:
            return youtubeSource_->getTrack(id);
        case SourceType::Podcast:
            return podcastSource_->getTrack(id);
        default:
            return Result<TrackInfo>("Unknown source type");
    }
}

Result<std::string> SourceManager::getStreamUrl(SourceType type, const std::string& id) {
    switch (type) {
        case SourceType::Local:
            return localSource_->getStreamUrl(id);
        case SourceType::YouTube:
            return youtubeSource_->getStreamUrl(id);
        case SourceType::Podcast:
            return podcastSource_->getStreamUrl(id);
        default:
            return Result<std::string>("Unknown source type");
    }
}

Result<Path> SourceManager::download(SourceType type, const std::string& id) {
    Path destination = dataDir_ / "downloads";
    std::filesystem::create_directories(destination);

    switch (type) {
        case SourceType::Local:
            return localSource_->download(id, destination);
        case SourceType::YouTube:
            return youtubeSource_->download(id, destination);
        case SourceType::Podcast:
            return podcastSource_->download(id, destination);
        default:
            return Result<Path>("Unknown source type");
    }
}

void SourceManager::rebuildDatabase() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    trackDatabase_.clear();

    // Re-scan local files
    if (localSource_) {
        localSource_->rescanAll();
        for (const auto& track : localSource_->allTracks()) {
            trackDatabase_[track.id] = track;
        }
    }

    saveDatabase();
}

size_t SourceManager::trackCount() const {
    std::lock_guard<std::mutex> lock(dbMutex_);
    return trackDatabase_.size();
}

std::vector<TrackInfo> SourceManager::getAllTracks() const {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<TrackInfo> result;
    result.reserve(trackDatabase_.size());
    for (const auto& [id, track] : trackDatabase_) {
        result.push_back(track);
    }
    return result;
}

std::vector<std::string> SourceManager::getAllArtists() const {
    if (localSource_) {
        return localSource_->allArtists();
    }
    return {};
}

std::vector<std::string> SourceManager::getAllAlbums() const {
    if (localSource_) {
        return localSource_->allAlbums();
    }
    return {};
}

std::vector<TrackInfo> SourceManager::getTracksByArtist(const std::string& artist) const {
    if (localSource_) {
        return localSource_->tracksByArtist(artist);
    }
    return {};
}

std::vector<TrackInfo> SourceManager::getTracksByAlbum(const std::string& album) const {
    if (localSource_) {
        return localSource_->tracksByAlbum(album);
    }
    return {};
}

void SourceManager::loadDatabase() {
    Path dbPath = dataDir_ / "library.yaml";
    if (!std::filesystem::exists(dbPath)) {
        return;
    }

    try {
        YAML::Node node = YAML::LoadFile(dbPath.string());

        std::lock_guard<std::mutex> lock(dbMutex_);
        trackDatabase_.clear();

        if (node["tracks"]) {
            for (const auto& trackNode : node["tracks"]) {
                TrackInfo track;
                track.id = trackNode["id"].as<std::string>();
                track.title = trackNode["title"].as<std::string>("");
                track.artist = trackNode["artist"].as<std::string>("");
                track.album = trackNode["album"].as<std::string>("");
                track.genre = trackNode["genre"].as<std::string>("");
                track.year = trackNode["year"].as<int>(0);
                track.trackNumber = trackNode["trackNumber"].as<int>(0);
                track.filePath = trackNode["filePath"].as<std::string>("");
                track.source = static_cast<SourceType>(trackNode["source"].as<int>(0));
                track.sourceId = trackNode["sourceId"].as<std::string>("");

                trackDatabase_[track.id] = track;
            }
        }
    } catch (...) {
        // Ignore errors, will rebuild database
    }
}

void SourceManager::saveDatabase() {
    Path dbPath = dataDir_ / "library.yaml";

    try {
        std::lock_guard<std::mutex> lock(dbMutex_);

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "version" << YAML::Value << 1;
        out << YAML::Key << "tracks" << YAML::Value << YAML::BeginSeq;

        for (const auto& [id, track] : trackDatabase_) {
            out << YAML::BeginMap;
            out << YAML::Key << "id" << YAML::Value << track.id;
            out << YAML::Key << "title" << YAML::Value << track.title;
            out << YAML::Key << "artist" << YAML::Value << track.artist;
            out << YAML::Key << "album" << YAML::Value << track.album;
            out << YAML::Key << "genre" << YAML::Value << track.genre;
            out << YAML::Key << "year" << YAML::Value << track.year;
            out << YAML::Key << "trackNumber" << YAML::Value << track.trackNumber;
            out << YAML::Key << "filePath" << YAML::Value << track.filePath.string();
            out << YAML::Key << "source" << YAML::Value << static_cast<int>(track.source);
            out << YAML::Key << "sourceId" << YAML::Value << track.sourceId;
            out << YAML::EndMap;
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream file(dbPath);
        file << out.c_str();

    } catch (...) {
        // Ignore save errors
    }
}

} // namespace soda
