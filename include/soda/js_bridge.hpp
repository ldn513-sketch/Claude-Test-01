/*
 * SODA Player - JavaScript Bridge
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 *
 * This provides the interface between the HTML/CSS skin and the C++ core.
 * All UI interactions go through this bridge.
 */

#ifndef SODA_JS_BRIDGE_HPP
#define SODA_JS_BRIDGE_HPP

#include "soda.hpp"
#include <json/json.h>

namespace soda {

class Application;

// Message from JS to C++
struct JsMessage {
    std::string method;
    Json::Value params;
    std::optional<std::string> callbackId;
};

// Response from C++ to JS
struct JsResponse {
    std::string callbackId;
    bool success = true;
    Json::Value result;
    std::string error;
};

class JsBridge {
public:
    explicit JsBridge(Application& app);
    ~JsBridge() = default;

    // Handle message from JavaScript
    JsResponse handleMessage(const std::string& messageJson);

    // Generate JavaScript API code
    std::string generateApiCode() const;

    // Push event to JavaScript
    std::string createEventJs(const Event& event) const;

    // Register custom handler (for plugins)
    using Handler = std::function<JsResponse(const Json::Value& params)>;
    void registerHandler(const std::string& method, Handler handler);
    void unregisterHandler(const std::string& method);

private:
    // Built-in handlers
    JsResponse handlePlay(const Json::Value& params);
    JsResponse handlePause(const Json::Value& params);
    JsResponse handleStop(const Json::Value& params);
    JsResponse handleTogglePlayPause(const Json::Value& params);
    JsResponse handleNext(const Json::Value& params);
    JsResponse handlePrevious(const Json::Value& params);
    JsResponse handleSeek(const Json::Value& params);
    JsResponse handleSetVolume(const Json::Value& params);
    JsResponse handleGetVolume(const Json::Value& params);
    JsResponse handleGetState(const Json::Value& params);
    JsResponse handleGetCurrentTrack(const Json::Value& params);
    JsResponse handleGetPosition(const Json::Value& params);

    // Queue handlers
    JsResponse handleQueueAdd(const Json::Value& params);
    JsResponse handleQueueRemove(const Json::Value& params);
    JsResponse handleQueueClear(const Json::Value& params);
    JsResponse handleQueueGet(const Json::Value& params);
    JsResponse handleQueueJumpTo(const Json::Value& params);
    JsResponse handleQueueShuffle(const Json::Value& params);

    // Library handlers
    JsResponse handleSearch(const Json::Value& params);
    JsResponse handleGetTracks(const Json::Value& params);
    JsResponse handleGetArtists(const Json::Value& params);
    JsResponse handleGetAlbums(const Json::Value& params);
    JsResponse handleGetTracksByArtist(const Json::Value& params);
    JsResponse handleGetTracksByAlbum(const Json::Value& params);

    // Playlist handlers
    JsResponse handleGetPlaylists(const Json::Value& params);
    JsResponse handleGetPlaylist(const Json::Value& params);
    JsResponse handleCreatePlaylist(const Json::Value& params);
    JsResponse handleDeletePlaylist(const Json::Value& params);
    JsResponse handleAddToPlaylist(const Json::Value& params);
    JsResponse handleRemoveFromPlaylist(const Json::Value& params);

    // YouTube handlers
    JsResponse handleYouTubeSearch(const Json::Value& params);
    JsResponse handleYouTubePlay(const Json::Value& params);
    JsResponse handleYouTubeImportPlaylist(const Json::Value& params);

    // Podcast handlers
    JsResponse handlePodcastSearch(const Json::Value& params);
    JsResponse handlePodcastSubscribe(const Json::Value& params);
    JsResponse handlePodcastUnsubscribe(const Json::Value& params);
    JsResponse handlePodcastGetFeeds(const Json::Value& params);
    JsResponse handlePodcastGetEpisodes(const Json::Value& params);

    // Settings handlers
    JsResponse handleGetSettings(const Json::Value& params);
    JsResponse handleSetSettings(const Json::Value& params);
    JsResponse handleGetSkins(const Json::Value& params);
    JsResponse handleSetSkin(const Json::Value& params);

    // Utility handlers
    JsResponse handleOpenExternal(const Json::Value& params);
    JsResponse handleShowNotification(const Json::Value& params);
    JsResponse handleMinimizeWindow(const Json::Value& params);
    JsResponse handleMaximizeWindow(const Json::Value& params);
    JsResponse handleCloseWindow(const Json::Value& params);
    JsResponse handleQuit(const Json::Value& params);

    // Helper methods
    Json::Value trackToJson(const TrackInfo& track) const;
    Json::Value playlistToJson(const PlaylistInfo& playlist) const;
    Json::Value searchResultToJson(const SearchResult& result) const;

    Application& app_;
    std::unordered_map<std::string, Handler> handlers_;
    mutable std::mutex mutex_;
};

} // namespace soda

#endif // SODA_JS_BRIDGE_HPP
