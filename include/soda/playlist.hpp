/*
 * SODA Player - Playlist Management
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_PLAYLIST_HPP
#define SODA_PLAYLIST_HPP

#include "soda.hpp"

namespace soda {

class Playlist {
public:
    Playlist() = default;
    explicit Playlist(const std::string& name);
    Playlist(const PlaylistInfo& info);
    ~Playlist() = default;

    // Basic info
    const std::string& id() const { return info_.id; }
    const std::string& name() const { return info_.name; }
    void setName(const std::string& name);

    const std::string& description() const { return info_.description; }
    void setDescription(const std::string& desc);

    // Track management
    void addTrack(const std::string& trackId);
    void addTracks(const std::vector<std::string>& trackIds);
    void removeTrack(const std::string& trackId);
    void removeTrackAt(size_t index);
    void moveTrack(size_t fromIndex, size_t toIndex);
    void clear();

    // Access
    const std::vector<std::string>& trackIds() const { return info_.trackIds; }
    size_t size() const { return info_.trackIds.size(); }
    bool isEmpty() const { return info_.trackIds.empty(); }
    bool contains(const std::string& trackId) const;

    // Cover
    std::optional<std::string> cover() const { return info_.coverPath; }
    void setCover(const std::string& path);

    // Source info (for YouTube/Podcast playlists)
    SourceType source() const { return info_.source; }
    const std::string& sourceId() const { return info_.sourceId; }
    void setSource(SourceType type, const std::string& sourceId);

    // Timestamps
    TimePoint createdAt() const { return info_.createdAt; }
    TimePoint modifiedAt() const { return info_.modifiedAt; }

    // Persistence (YAML format)
    Result<void> save(const Path& directory) const;
    static Result<Playlist> load(const Path& filePath);

    // Export to standard formats
    Result<void> exportToM3U(const Path& filePath) const;
    static Result<Playlist> importFromM3U(const Path& filePath);

    // Get full info
    const PlaylistInfo& info() const { return info_; }

private:
    void generateId();
    void updateModifiedTime();

    PlaylistInfo info_;
};

// Playlist manager for handling multiple playlists
class PlaylistManager {
public:
    explicit PlaylistManager(const Path& playlistDir);
    ~PlaylistManager() = default;

    // Load all playlists from directory
    Result<void> loadAll();

    // CRUD operations
    Playlist& create(const std::string& name);
    std::optional<Playlist*> get(const std::string& id);
    std::optional<const Playlist*> get(const std::string& id) const;
    bool remove(const std::string& id);
    Result<void> save(const std::string& id);
    Result<void> saveAll();

    // Access
    std::vector<PlaylistInfo> list() const;
    size_t count() const { return playlists_.size(); }

    // Search
    std::vector<PlaylistInfo> search(const std::string& query) const;

private:
    Path playlistDir_;
    std::unordered_map<std::string, Playlist> playlists_;
    mutable std::mutex mutex_;
};

} // namespace soda

#endif // SODA_PLAYLIST_HPP
