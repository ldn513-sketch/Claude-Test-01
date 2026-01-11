/*
 * SODA Player - Local Source Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/local_source.hpp>
#include <soda/metadata_reader.hpp>
#include <soda/event_bus.hpp>
#include <soda/string_utils.hpp>
#include <algorithm>
#include <fstream>

namespace soda {

const std::vector<std::string> LocalSource::SUPPORTED_EXTENSIONS = {
    ".mp3", ".m4a", ".flac", ".ogg", ".opus", ".wav", ".aac"
};

LocalSource::LocalSource(EventBus& eventBus)
    : eventBus_(eventBus)
    , metadataReader_(std::make_unique<MetadataReader>())
{
}

LocalSource::~LocalSource() {
    stopWatching();
    if (scanThread_.joinable()) {
        scanThread_.join();
    }
}

std::vector<SearchResult> LocalSource::search(const std::string& query) {
    std::vector<SearchResult> results;
    std::lock_guard<std::mutex> lock(mutex_);

    std::string lowerQuery = string_utils::toLower(query);

    for (const auto& [id, track] : tracks_) {
        if (string_utils::containsIgnoreCase(track.title, query) ||
            string_utils::containsIgnoreCase(track.artist, query) ||
            string_utils::containsIgnoreCase(track.album, query)) {

            SearchResult result;
            result.id = track.id;
            result.title = track.title;
            result.subtitle = track.artist + " - " + track.album;
            result.source = SourceType::Local;
            result.sourceId = track.filePath.string();
            result.duration = track.duration;
            result.isPlaylist = false;

            results.push_back(result);
        }
    }

    return results;
}

Result<TrackInfo> LocalSource::getTrack(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tracks_.find(id);
    if (it != tracks_.end()) {
        return Result<TrackInfo>(it->second);
    }
    return Result<TrackInfo>("Track not found: " + id);
}

Result<std::string> LocalSource::getStreamUrl(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tracks_.find(id);
    if (it != tracks_.end()) {
        return Result<std::string>("file://" + it->second.filePath.string());
    }
    return Result<std::string>("Track not found: " + id);
}

Result<Path> LocalSource::download(const std::string& id, const Path& destination) {
    // Local files don't need downloading
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tracks_.find(id);
    if (it != tracks_.end()) {
        return Result<Path>(it->second.filePath);
    }
    return Result<Path>("Track not found: " + id);
}

void LocalSource::addFolder(const Path& folder) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (std::find(watchedFolders_.begin(), watchedFolders_.end(), folder) == watchedFolders_.end()) {
        watchedFolders_.push_back(folder);
    }
}

void LocalSource::removeFolder(const Path& folder) {
    std::lock_guard<std::mutex> lock(mutex_);
    watchedFolders_.erase(
        std::remove(watchedFolders_.begin(), watchedFolders_.end(), folder),
        watchedFolders_.end()
    );
}

std::vector<Path> LocalSource::watchedFolders() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return watchedFolders_;
}

void LocalSource::scan() {
    if (scanning_.load()) {
        return;
    }

    scanning_.store(true);

    if (scanThread_.joinable()) {
        scanThread_.join();
    }

    scanThread_ = std::thread([this]() {
        std::vector<Path> folders;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            folders = watchedFolders_;
        }

        for (const auto& folder : folders) {
            scanFolder(folder);
        }

        scanning_.store(false);
        eventBus_.publishAsync(Event{EventType::SourceUpdated, std::string("local")});
    });
}

void LocalSource::scanFolder(const Path& folder) {
    if (!std::filesystem::exists(folder)) {
        return;
    }

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(folder)) {
            if (!scanning_.load()) {
                break;
            }

            if (entry.is_regular_file() && isSupportedFormat(entry.path())) {
                indexFile(entry.path());
            }
        }
    } catch (const std::exception& e) {
        // Log error but continue
    }
}

void LocalSource::rescanAll() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tracks_.clear();
        artistTracks_.clear();
        albumTracks_.clear();
        genreTracks_.clear();
    }
    scan();
}

void LocalSource::startWatching() {
    if (watching_.load()) {
        return;
    }

    watching_.store(true);
    // File watching implementation would go here
    // Using inotify on Linux
}

void LocalSource::stopWatching() {
    watching_.store(false);
    if (watchThread_.joinable()) {
        watchThread_.join();
    }
}

std::vector<TrackInfo> LocalSource::allTracks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TrackInfo> result;
    result.reserve(tracks_.size());
    for (const auto& [id, track] : tracks_) {
        result.push_back(track);
    }
    return result;
}

std::vector<std::string> LocalSource::allArtists() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    result.reserve(artistTracks_.size());
    for (const auto& [artist, _] : artistTracks_) {
        result.push_back(artist);
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<std::string> LocalSource::allAlbums() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    result.reserve(albumTracks_.size());
    for (const auto& [album, _] : albumTracks_) {
        result.push_back(album);
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<std::string> LocalSource::allGenres() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    result.reserve(genreTracks_.size());
    for (const auto& [genre, _] : genreTracks_) {
        result.push_back(genre);
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<TrackInfo> LocalSource::tracksByArtist(const std::string& artist) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TrackInfo> result;

    auto it = artistTracks_.find(artist);
    if (it != artistTracks_.end()) {
        for (const auto& id : it->second) {
            auto trackIt = tracks_.find(id);
            if (trackIt != tracks_.end()) {
                result.push_back(trackIt->second);
            }
        }
    }

    return result;
}

std::vector<TrackInfo> LocalSource::tracksByAlbum(const std::string& album) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TrackInfo> result;

    auto it = albumTracks_.find(album);
    if (it != albumTracks_.end()) {
        for (const auto& id : it->second) {
            auto trackIt = tracks_.find(id);
            if (trackIt != tracks_.end()) {
                result.push_back(trackIt->second);
            }
        }
    }

    // Sort by track number
    std::sort(result.begin(), result.end(),
              [](const TrackInfo& a, const TrackInfo& b) {
                  return a.trackNumber < b.trackNumber;
              });

    return result;
}

std::vector<TrackInfo> LocalSource::tracksByGenre(const std::string& genre) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TrackInfo> result;

    auto it = genreTracks_.find(genre);
    if (it != genreTracks_.end()) {
        for (const auto& id : it->second) {
            auto trackIt = tracks_.find(id);
            if (trackIt != tracks_.end()) {
                result.push_back(trackIt->second);
            }
        }
    }

    return result;
}

std::vector<TrackInfo> LocalSource::tracksByYear(int year) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TrackInfo> result;

    for (const auto& [id, track] : tracks_) {
        if (track.year == year) {
            result.push_back(track);
        }
    }

    return result;
}

std::vector<TrackInfo> LocalSource::tracksByDecade(const std::string& decade) const {
    // Parse decade string like "1990s" to get start year
    int startYear = 0;
    if (decade.size() >= 4) {
        try {
            startYear = std::stoi(decade.substr(0, 4));
            startYear = (startYear / 10) * 10; // Round down to decade
        } catch (...) {
            return {};
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TrackInfo> result;

    for (const auto& [id, track] : tracks_) {
        if (track.year >= startYear && track.year < startYear + 10) {
            result.push_back(track);
        }
    }

    return result;
}

std::vector<TrackInfo> LocalSource::recentlyAdded(size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TrackInfo> result;
    result.reserve(std::min(limit, tracks_.size()));

    // TODO: Track addition time and sort by it
    // For now, just return the first N tracks
    size_t count = 0;
    for (const auto& [id, track] : tracks_) {
        if (count++ >= limit) break;
        result.push_back(track);
    }

    return result;
}

std::vector<TrackInfo> LocalSource::recentlyPlayed(size_t limit) const {
    // TODO: Track play history
    return {};
}

std::vector<TrackInfo> LocalSource::mostPlayed(size_t limit) const {
    // TODO: Track play counts
    return {};
}

size_t LocalSource::totalTracks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tracks_.size();
}

size_t LocalSource::totalArtists() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return artistTracks_.size();
}

size_t LocalSource::totalAlbums() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return albumTracks_.size();
}

Duration LocalSource::totalDuration() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Duration total{0};
    for (const auto& [id, track] : tracks_) {
        total += track.duration;
    }
    return total;
}

void LocalSource::indexFile(const Path& file) {
    auto result = metadataReader_->read(file);
    if (!result.ok()) {
        // Create basic track info from filename
        TrackInfo track;
        track.id = generateId(file);
        track.filePath = file;
        track.title = file.stem().string();
        track.source = SourceType::Local;
        track.format = AudioDecoder::detectFormat(file);

        std::lock_guard<std::mutex> lock(mutex_);
        tracks_[track.id] = track;
        return;
    }

    TrackInfo track = result.value();
    track.id = generateId(file);
    track.source = SourceType::Local;

    std::lock_guard<std::mutex> lock(mutex_);
    tracks_[track.id] = track;

    // Update indexes
    if (!track.artist.empty()) {
        artistTracks_[track.artist].push_back(track.id);
    }
    if (!track.album.empty()) {
        albumTracks_[track.album].push_back(track.id);
    }
    if (!track.genre.empty()) {
        genreTracks_[track.genre].push_back(track.id);
    }
}

void LocalSource::removeFile(const Path& file) {
    std::string id = generateId(file);

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tracks_.find(id);
    if (it == tracks_.end()) {
        return;
    }

    const TrackInfo& track = it->second;

    // Remove from indexes
    if (!track.artist.empty()) {
        auto& artistList = artistTracks_[track.artist];
        artistList.erase(std::remove(artistList.begin(), artistList.end(), id), artistList.end());
        if (artistList.empty()) {
            artistTracks_.erase(track.artist);
        }
    }
    if (!track.album.empty()) {
        auto& albumList = albumTracks_[track.album];
        albumList.erase(std::remove(albumList.begin(), albumList.end(), id), albumList.end());
        if (albumList.empty()) {
            albumTracks_.erase(track.album);
        }
    }
    if (!track.genre.empty()) {
        auto& genreList = genreTracks_[track.genre];
        genreList.erase(std::remove(genreList.begin(), genreList.end(), id), genreList.end());
        if (genreList.empty()) {
            genreTracks_.erase(track.genre);
        }
    }

    tracks_.erase(it);
}

std::string LocalSource::generateId(const Path& file) {
    return "local:" + string_utils::md5(file.string());
}

bool LocalSource::isSupportedFormat(const Path& file) const {
    std::string ext = file.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return std::find(SUPPORTED_EXTENSIONS.begin(), SUPPORTED_EXTENSIONS.end(), ext)
           != SUPPORTED_EXTENSIONS.end();
}

} // namespace soda
