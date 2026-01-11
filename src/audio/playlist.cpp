/*
 * SODA Player - Playlist Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/playlist.hpp>
#include <soda/string_utils.hpp>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <algorithm>

namespace soda {

Playlist::Playlist(const std::string& name) {
    info_.name = name;
    info_.createdAt = Clock::now();
    info_.modifiedAt = info_.createdAt;
    generateId();
}

Playlist::Playlist(const PlaylistInfo& info) : info_(info) {
    if (info_.id.empty()) {
        generateId();
    }
}

void Playlist::generateId() {
    info_.id = string_utils::generateUuid();
}

void Playlist::updateModifiedTime() {
    info_.modifiedAt = Clock::now();
}

void Playlist::setName(const std::string& name) {
    info_.name = name;
    updateModifiedTime();
}

void Playlist::setDescription(const std::string& desc) {
    info_.description = desc;
    updateModifiedTime();
}

void Playlist::addTrack(const std::string& trackId) {
    info_.trackIds.push_back(trackId);
    updateModifiedTime();
}

void Playlist::addTracks(const std::vector<std::string>& trackIds) {
    info_.trackIds.insert(info_.trackIds.end(), trackIds.begin(), trackIds.end());
    updateModifiedTime();
}

void Playlist::removeTrack(const std::string& trackId) {
    auto it = std::find(info_.trackIds.begin(), info_.trackIds.end(), trackId);
    if (it != info_.trackIds.end()) {
        info_.trackIds.erase(it);
        updateModifiedTime();
    }
}

void Playlist::removeTrackAt(size_t index) {
    if (index < info_.trackIds.size()) {
        info_.trackIds.erase(info_.trackIds.begin() + index);
        updateModifiedTime();
    }
}

void Playlist::moveTrack(size_t fromIndex, size_t toIndex) {
    if (fromIndex >= info_.trackIds.size() || toIndex >= info_.trackIds.size()) {
        return;
    }

    std::string trackId = info_.trackIds[fromIndex];
    info_.trackIds.erase(info_.trackIds.begin() + fromIndex);

    if (toIndex > fromIndex) {
        --toIndex;
    }

    info_.trackIds.insert(info_.trackIds.begin() + toIndex, trackId);
    updateModifiedTime();
}

void Playlist::clear() {
    info_.trackIds.clear();
    updateModifiedTime();
}

bool Playlist::contains(const std::string& trackId) const {
    return std::find(info_.trackIds.begin(), info_.trackIds.end(), trackId) != info_.trackIds.end();
}

void Playlist::setCover(const std::string& path) {
    info_.coverPath = path;
    updateModifiedTime();
}

void Playlist::setSource(SourceType type, const std::string& sourceId) {
    info_.source = type;
    info_.sourceId = sourceId;
    updateModifiedTime();
}

Result<void> Playlist::save(const Path& directory) const {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "id" << YAML::Value << info_.id;
    out << YAML::Key << "name" << YAML::Value << info_.name;
    out << YAML::Key << "description" << YAML::Value << info_.description;

    if (info_.coverPath) {
        out << YAML::Key << "cover" << YAML::Value << *info_.coverPath;
    }

    out << YAML::Key << "source" << YAML::Value << static_cast<int>(info_.source);

    if (!info_.sourceId.empty()) {
        out << YAML::Key << "sourceId" << YAML::Value << info_.sourceId;
    }

    out << YAML::Key << "createdAt" << YAML::Value
        << std::chrono::duration_cast<std::chrono::seconds>(
               info_.createdAt.time_since_epoch()).count();
    out << YAML::Key << "modifiedAt" << YAML::Value
        << std::chrono::duration_cast<std::chrono::seconds>(
               info_.modifiedAt.time_since_epoch()).count();

    out << YAML::Key << "tracks" << YAML::Value << YAML::BeginSeq;
    for (const auto& trackId : info_.trackIds) {
        out << trackId;
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;

    Path filePath = directory / (info_.id + ".yaml");
    std::ofstream file(filePath);
    if (!file) {
        return Result<void>("Failed to open file for writing: " + filePath.string());
    }

    file << out.c_str();
    return Result<void>();
}

Result<Playlist> Playlist::load(const Path& filePath) {
    try {
        YAML::Node node = YAML::LoadFile(filePath.string());

        PlaylistInfo info;
        info.id = node["id"].as<std::string>();
        info.name = node["name"].as<std::string>();

        if (node["description"]) {
            info.description = node["description"].as<std::string>();
        }

        if (node["cover"]) {
            info.coverPath = node["cover"].as<std::string>();
        }

        if (node["source"]) {
            info.source = static_cast<SourceType>(node["source"].as<int>());
        }

        if (node["sourceId"]) {
            info.sourceId = node["sourceId"].as<std::string>();
        }

        if (node["createdAt"]) {
            info.createdAt = TimePoint(std::chrono::seconds(node["createdAt"].as<int64_t>()));
        }

        if (node["modifiedAt"]) {
            info.modifiedAt = TimePoint(std::chrono::seconds(node["modifiedAt"].as<int64_t>()));
        }

        if (node["tracks"]) {
            for (const auto& track : node["tracks"]) {
                info.trackIds.push_back(track.as<std::string>());
            }
        }

        return Result<Playlist>(Playlist(info));

    } catch (const std::exception& e) {
        return Result<Playlist>("Failed to load playlist: " + std::string(e.what()));
    }
}

Result<void> Playlist::exportToM3U(const Path& filePath) const {
    std::ofstream file(filePath);
    if (!file) {
        return Result<void>("Failed to open file for writing");
    }

    file << "#EXTM3U\n";
    file << "#PLAYLIST:" << info_.name << "\n";

    for (const auto& trackId : info_.trackIds) {
        // In a real implementation, you'd look up the actual track path
        file << trackId << "\n";
    }

    return Result<void>();
}

Result<Playlist> Playlist::importFromM3U(const Path& filePath) {
    std::ifstream file(filePath);
    if (!file) {
        return Result<Playlist>("Failed to open M3U file");
    }

    std::string name = filePath.stem().string();
    std::vector<std::string> trackIds;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            if (line.rfind("#PLAYLIST:", 0) == 0) {
                name = line.substr(10);
            }
            continue;
        }
        trackIds.push_back(line);
    }

    Playlist playlist(name);
    playlist.addTracks(trackIds);
    return Result<Playlist>(playlist);
}

// PlaylistManager implementation

PlaylistManager::PlaylistManager(const Path& playlistDir)
    : playlistDir_(playlistDir)
{
    std::filesystem::create_directories(playlistDir_);
}

Result<void> PlaylistManager::loadAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    playlists_.clear();

    try {
        for (const auto& entry : std::filesystem::directory_iterator(playlistDir_)) {
            if (entry.path().extension() == ".yaml") {
                auto result = Playlist::load(entry.path());
                if (result.ok()) {
                    playlists_[result.value().id()] = result.value();
                }
            }
        }
        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>("Error loading playlists: " + std::string(e.what()));
    }
}

Playlist& PlaylistManager::create(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    Playlist playlist(name);
    std::string id = playlist.id();
    playlists_[id] = std::move(playlist);
    return playlists_[id];
}

std::optional<Playlist*> PlaylistManager::get(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = playlists_.find(id);
    if (it != playlists_.end()) {
        return &it->second;
    }
    return std::nullopt;
}

std::optional<const Playlist*> PlaylistManager::get(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = playlists_.find(id);
    if (it != playlists_.end()) {
        return &it->second;
    }
    return std::nullopt;
}

bool PlaylistManager::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = playlists_.find(id);
    if (it == playlists_.end()) {
        return false;
    }

    Path filePath = playlistDir_ / (id + ".yaml");
    std::filesystem::remove(filePath);
    playlists_.erase(it);
    return true;
}

Result<void> PlaylistManager::save(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = playlists_.find(id);
    if (it == playlists_.end()) {
        return Result<void>("Playlist not found");
    }
    return it->second.save(playlistDir_);
}

Result<void> PlaylistManager::saveAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, playlist] : playlists_) {
        auto result = playlist.save(playlistDir_);
        if (!result.ok()) {
            return result;
        }
    }
    return Result<void>();
}

std::vector<PlaylistInfo> PlaylistManager::list() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PlaylistInfo> result;
    result.reserve(playlists_.size());
    for (const auto& [id, playlist] : playlists_) {
        result.push_back(playlist.info());
    }
    return result;
}

std::vector<PlaylistInfo> PlaylistManager::search(const std::string& query) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PlaylistInfo> result;

    std::string lowerQuery = string_utils::toLower(query);

    for (const auto& [id, playlist] : playlists_) {
        if (string_utils::containsIgnoreCase(playlist.name(), query) ||
            string_utils::containsIgnoreCase(playlist.description(), query)) {
            result.push_back(playlist.info());
        }
    }

    return result;
}

} // namespace soda
