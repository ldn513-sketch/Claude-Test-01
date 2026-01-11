/*
 * SODA Player - JavaScript Bridge Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/js_bridge.hpp>
#include <soda/application.hpp>
#include <soda/audio_engine.hpp>
#include <soda/source_manager.hpp>
#include <soda/config_manager.hpp>
#include <soda/skin_manager.hpp>
#include <soda/string_utils.hpp>

namespace soda {

JsBridge::JsBridge(Application& app)
    : app_(app)
{
    // Register built-in handlers
    handlers_["play"] = [this](const Json::Value& p) { return handlePlay(p); };
    handlers_["pause"] = [this](const Json::Value& p) { return handlePause(p); };
    handlers_["stop"] = [this](const Json::Value& p) { return handleStop(p); };
    handlers_["togglePlayPause"] = [this](const Json::Value& p) { return handleTogglePlayPause(p); };
    handlers_["next"] = [this](const Json::Value& p) { return handleNext(p); };
    handlers_["previous"] = [this](const Json::Value& p) { return handlePrevious(p); };
    handlers_["seek"] = [this](const Json::Value& p) { return handleSeek(p); };
    handlers_["setVolume"] = [this](const Json::Value& p) { return handleSetVolume(p); };
    handlers_["getVolume"] = [this](const Json::Value& p) { return handleGetVolume(p); };
    handlers_["getState"] = [this](const Json::Value& p) { return handleGetState(p); };
    handlers_["getCurrentTrack"] = [this](const Json::Value& p) { return handleGetCurrentTrack(p); };
    handlers_["getPosition"] = [this](const Json::Value& p) { return handleGetPosition(p); };

    handlers_["queueAdd"] = [this](const Json::Value& p) { return handleQueueAdd(p); };
    handlers_["queueRemove"] = [this](const Json::Value& p) { return handleQueueRemove(p); };
    handlers_["queueClear"] = [this](const Json::Value& p) { return handleQueueClear(p); };
    handlers_["queueGet"] = [this](const Json::Value& p) { return handleQueueGet(p); };
    handlers_["queueJumpTo"] = [this](const Json::Value& p) { return handleQueueJumpTo(p); };
    handlers_["queueShuffle"] = [this](const Json::Value& p) { return handleQueueShuffle(p); };

    handlers_["search"] = [this](const Json::Value& p) { return handleSearch(p); };
    handlers_["getTracks"] = [this](const Json::Value& p) { return handleGetTracks(p); };
    handlers_["getArtists"] = [this](const Json::Value& p) { return handleGetArtists(p); };
    handlers_["getAlbums"] = [this](const Json::Value& p) { return handleGetAlbums(p); };
    handlers_["getTracksByArtist"] = [this](const Json::Value& p) { return handleGetTracksByArtist(p); };
    handlers_["getTracksByAlbum"] = [this](const Json::Value& p) { return handleGetTracksByAlbum(p); };

    handlers_["getPlaylists"] = [this](const Json::Value& p) { return handleGetPlaylists(p); };
    handlers_["getPlaylist"] = [this](const Json::Value& p) { return handleGetPlaylist(p); };
    handlers_["createPlaylist"] = [this](const Json::Value& p) { return handleCreatePlaylist(p); };
    handlers_["deletePlaylist"] = [this](const Json::Value& p) { return handleDeletePlaylist(p); };
    handlers_["addToPlaylist"] = [this](const Json::Value& p) { return handleAddToPlaylist(p); };

    handlers_["getSettings"] = [this](const Json::Value& p) { return handleGetSettings(p); };
    handlers_["setSettings"] = [this](const Json::Value& p) { return handleSetSettings(p); };
    handlers_["getSkins"] = [this](const Json::Value& p) { return handleGetSkins(p); };
    handlers_["setSkin"] = [this](const Json::Value& p) { return handleSetSkin(p); };

    handlers_["minimizeWindow"] = [this](const Json::Value& p) { return handleMinimizeWindow(p); };
    handlers_["maximizeWindow"] = [this](const Json::Value& p) { return handleMaximizeWindow(p); };
    handlers_["closeWindow"] = [this](const Json::Value& p) { return handleCloseWindow(p); };
    handlers_["quit"] = [this](const Json::Value& p) { return handleQuit(p); };
}

JsResponse JsBridge::handleMessage(const std::string& messageJson) {
    JsResponse response;

    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        std::istringstream stream(messageJson);

        if (!Json::parseFromStream(builder, stream, &root, &errors)) {
            response.success = false;
            response.error = "Invalid JSON: " + errors;
            return response;
        }

        std::string method = root["method"].asString();
        Json::Value params = root["params"];

        if (root.isMember("callbackId")) {
            response.callbackId = root["callbackId"].asString();
        }

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(method);
        if (it != handlers_.end()) {
            return it->second(params);
        }

        response.success = false;
        response.error = "Unknown method: " + method;

    } catch (const std::exception& e) {
        response.success = false;
        response.error = std::string("Error: ") + e.what();
    }

    return response;
}

std::string JsBridge::generateApiCode() const {
    // The API code is generated in SkinManager
    return "";
}

std::string JsBridge::createEventJs(const Event& event) const {
    Json::Value eventObj;

    switch (event.type) {
        case EventType::PlaybackStarted:
            eventObj["type"] = "playbackStarted";
            if (auto* track = std::get_if<TrackInfo>(&event.data)) {
                eventObj["data"] = trackToJson(*track);
            }
            break;

        case EventType::PlaybackPaused:
            eventObj["type"] = "playbackPaused";
            break;

        case EventType::PlaybackStopped:
            eventObj["type"] = "playbackStopped";
            break;

        case EventType::PlaybackProgress:
            eventObj["type"] = "playbackProgress";
            if (auto* pos = std::get_if<double>(&event.data)) {
                eventObj["data"]["position"] = *pos;
            }
            break;

        case EventType::TrackChanged:
            eventObj["type"] = "trackChanged";
            if (auto* track = std::get_if<TrackInfo>(&event.data)) {
                eventObj["data"] = trackToJson(*track);
            }
            break;

        case EventType::QueueChanged:
            eventObj["type"] = "queueChanged";
            break;

        case EventType::VolumeChanged:
            eventObj["type"] = "volumeChanged";
            if (auto* vol = std::get_if<double>(&event.data)) {
                eventObj["data"]["volume"] = *vol;
            }
            break;

        case EventType::Error:
            eventObj["type"] = "error";
            if (auto* msg = std::get_if<std::string>(&event.data)) {
                eventObj["data"]["message"] = *msg;
            }
            break;

        default:
            eventObj["type"] = "unknown";
    }

    Json::StreamWriterBuilder writer;
    std::string eventJson = Json::writeString(writer, eventObj);

    return "if (window.soda && window.soda.onEvent) { window.soda.onEvent(" + eventJson + "); }";
}

void JsBridge::registerHandler(const std::string& method, Handler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[method] = std::move(handler);
}

void JsBridge::unregisterHandler(const std::string& method) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_.erase(method);
}

// Playback handlers
JsResponse JsBridge::handlePlay(const Json::Value& params) {
    JsResponse response;
    auto result = app_.audioEngine().play();
    response.success = result.ok();
    if (!result.ok()) {
        response.error = result.error();
    }
    return response;
}

JsResponse JsBridge::handlePause(const Json::Value& params) {
    JsResponse response;
    auto result = app_.audioEngine().pause();
    response.success = result.ok();
    return response;
}

JsResponse JsBridge::handleStop(const Json::Value& params) {
    JsResponse response;
    auto result = app_.audioEngine().stop();
    response.success = result.ok();
    return response;
}

JsResponse JsBridge::handleTogglePlayPause(const Json::Value& params) {
    JsResponse response;
    auto result = app_.audioEngine().togglePlayPause();
    response.success = result.ok();
    return response;
}

JsResponse JsBridge::handleNext(const Json::Value& params) {
    JsResponse response;
    auto result = app_.audioEngine().playNext();
    response.success = result.ok();
    return response;
}

JsResponse JsBridge::handlePrevious(const Json::Value& params) {
    JsResponse response;
    auto result = app_.audioEngine().playPrevious();
    response.success = result.ok();
    return response;
}

JsResponse JsBridge::handleSeek(const Json::Value& params) {
    JsResponse response;
    int64_t position = params["position"].asInt64();
    auto result = app_.audioEngine().seek(Duration(position));
    response.success = result.ok();
    return response;
}

JsResponse JsBridge::handleSetVolume(const Json::Value& params) {
    JsResponse response;
    float volume = static_cast<float>(params["volume"].asDouble());
    app_.audioEngine().setVolume(volume);
    response.success = true;
    return response;
}

JsResponse JsBridge::handleGetVolume(const Json::Value& params) {
    JsResponse response;
    response.success = true;
    response.result["volume"] = app_.audioEngine().volume();
    return response;
}

JsResponse JsBridge::handleGetState(const Json::Value& params) {
    JsResponse response;
    response.success = true;

    switch (app_.audioEngine().state()) {
        case PlaybackState::Stopped: response.result["state"] = "stopped"; break;
        case PlaybackState::Playing: response.result["state"] = "playing"; break;
        case PlaybackState::Paused: response.result["state"] = "paused"; break;
        case PlaybackState::Buffering: response.result["state"] = "buffering"; break;
    }

    return response;
}

JsResponse JsBridge::handleGetCurrentTrack(const Json::Value& params) {
    JsResponse response;
    auto track = app_.audioEngine().currentTrack();
    if (track) {
        response.success = true;
        response.result = trackToJson(*track);
    } else {
        response.success = true;
        response.result = Json::nullValue;
    }
    return response;
}

JsResponse JsBridge::handleGetPosition(const Json::Value& params) {
    JsResponse response;
    response.success = true;
    response.result["position"] = app_.audioEngine().position().count();
    response.result["duration"] = app_.audioEngine().duration().count();
    return response;
}

// Queue handlers
JsResponse JsBridge::handleQueueAdd(const Json::Value& params) {
    JsResponse response;
    std::string trackId = params["trackId"].asString();

    auto trackResult = app_.sources().getTrack(SourceType::Local, trackId);
    if (trackResult.ok()) {
        app_.audioEngine().queue().add(trackResult.value());
        response.success = true;
    } else {
        response.success = false;
        response.error = trackResult.error();
    }
    return response;
}

JsResponse JsBridge::handleQueueRemove(const Json::Value& params) {
    JsResponse response;
    size_t index = params["index"].asUInt();
    app_.audioEngine().queue().remove(index);
    response.success = true;
    return response;
}

JsResponse JsBridge::handleQueueClear(const Json::Value& params) {
    JsResponse response;
    app_.audioEngine().queue().clear();
    response.success = true;
    return response;
}

JsResponse JsBridge::handleQueueGet(const Json::Value& params) {
    JsResponse response;
    response.success = true;

    const auto& tracks = app_.audioEngine().queue().tracks();
    Json::Value tracksArray(Json::arrayValue);

    for (const auto& track : tracks) {
        tracksArray.append(trackToJson(track));
    }

    response.result["tracks"] = tracksArray;
    response.result["currentIndex"] = static_cast<int>(app_.audioEngine().queue().currentIndex());
    response.result["shuffled"] = app_.audioEngine().queue().isShuffled();

    return response;
}

JsResponse JsBridge::handleQueueJumpTo(const Json::Value& params) {
    JsResponse response;
    size_t index = params["index"].asUInt();
    app_.audioEngine().queue().jumpTo(index);
    app_.audioEngine().play();
    response.success = true;
    return response;
}

JsResponse JsBridge::handleQueueShuffle(const Json::Value& params) {
    JsResponse response;
    if (app_.audioEngine().queue().isShuffled()) {
        app_.audioEngine().queue().unshuffle();
    } else {
        app_.audioEngine().queue().shuffle();
    }
    response.success = true;
    response.result["shuffled"] = app_.audioEngine().queue().isShuffled();
    return response;
}

// Library handlers
JsResponse JsBridge::handleSearch(const Json::Value& params) {
    JsResponse response;
    std::string query = params["query"].asString();

    auto results = app_.search(query);

    Json::Value resultsArray(Json::arrayValue);
    for (const auto& result : results) {
        resultsArray.append(searchResultToJson(result));
    }

    response.success = true;
    response.result = resultsArray;
    return response;
}

JsResponse JsBridge::handleGetTracks(const Json::Value& params) {
    JsResponse response;
    auto tracks = app_.sources().getAllTracks();

    Json::Value tracksArray(Json::arrayValue);
    for (const auto& track : tracks) {
        tracksArray.append(trackToJson(track));
    }

    response.success = true;
    response.result = tracksArray;
    return response;
}

JsResponse JsBridge::handleGetArtists(const Json::Value& params) {
    JsResponse response;
    auto artists = app_.sources().getAllArtists();

    Json::Value artistsArray(Json::arrayValue);
    for (const auto& artist : artists) {
        artistsArray.append(artist);
    }

    response.success = true;
    response.result = artistsArray;
    return response;
}

JsResponse JsBridge::handleGetAlbums(const Json::Value& params) {
    JsResponse response;
    auto albums = app_.sources().getAllAlbums();

    Json::Value albumsArray(Json::arrayValue);
    for (const auto& album : albums) {
        albumsArray.append(album);
    }

    response.success = true;
    response.result = albumsArray;
    return response;
}

JsResponse JsBridge::handleGetTracksByArtist(const Json::Value& params) {
    JsResponse response;
    std::string artist = params["artist"].asString();
    auto tracks = app_.sources().getTracksByArtist(artist);

    Json::Value tracksArray(Json::arrayValue);
    for (const auto& track : tracks) {
        tracksArray.append(trackToJson(track));
    }

    response.success = true;
    response.result = tracksArray;
    return response;
}

JsResponse JsBridge::handleGetTracksByAlbum(const Json::Value& params) {
    JsResponse response;
    std::string album = params["album"].asString();
    auto tracks = app_.sources().getTracksByAlbum(album);

    Json::Value tracksArray(Json::arrayValue);
    for (const auto& track : tracks) {
        tracksArray.append(trackToJson(track));
    }

    response.success = true;
    response.result = tracksArray;
    return response;
}

// Settings handlers
JsResponse JsBridge::handleGetSettings(const Json::Value& params) {
    JsResponse response;
    response.success = true;

    const auto& settings = app_.config().settings();
    response.result["volume"] = settings.volume;
    response.result["shuffle"] = settings.shuffle;
    response.result["currentSkin"] = settings.currentSkin;
    response.result["showNotifications"] = settings.showNotifications;

    return response;
}

JsResponse JsBridge::handleSetSettings(const Json::Value& params) {
    JsResponse response;
    auto& settings = app_.config().settings();

    if (params.isMember("volume")) {
        settings.volume = static_cast<float>(params["volume"].asDouble());
    }
    if (params.isMember("shuffle")) {
        settings.shuffle = params["shuffle"].asBool();
    }

    app_.config().save();
    response.success = true;
    return response;
}

JsResponse JsBridge::handleGetSkins(const Json::Value& params) {
    JsResponse response;
    auto skins = app_.skins().availableSkins();

    Json::Value skinsArray(Json::arrayValue);
    for (const auto& skin : skins) {
        Json::Value skinObj;
        skinObj["id"] = skin.id;
        skinObj["name"] = skin.name;
        skinObj["author"] = skin.author;
        skinObj["description"] = skin.description;
        skinsArray.append(skinObj);
    }

    response.success = true;
    response.result = skinsArray;
    return response;
}

JsResponse JsBridge::handleSetSkin(const Json::Value& params) {
    JsResponse response;
    std::string skinId = params["skinId"].asString();
    auto result = app_.skins().setSkin(skinId);
    response.success = result.ok();
    if (!result.ok()) {
        response.error = result.error();
    }
    return response;
}

// Window handlers
JsResponse JsBridge::handleMinimizeWindow(const Json::Value& params) {
    JsResponse response;
    response.success = true;
    return response;
}

JsResponse JsBridge::handleMaximizeWindow(const Json::Value& params) {
    JsResponse response;
    response.success = true;
    return response;
}

JsResponse JsBridge::handleCloseWindow(const Json::Value& params) {
    JsResponse response;
    response.success = true;
    return response;
}

JsResponse JsBridge::handleQuit(const Json::Value& params) {
    JsResponse response;
    app_.quit();
    response.success = true;
    return response;
}

// Playlist handlers (stubs)
JsResponse JsBridge::handleGetPlaylists(const Json::Value& params) {
    JsResponse response;
    response.success = true;
    response.result = Json::arrayValue;
    return response;
}

JsResponse JsBridge::handleGetPlaylist(const Json::Value& params) {
    JsResponse response;
    response.success = false;
    response.error = "Not implemented";
    return response;
}

JsResponse JsBridge::handleCreatePlaylist(const Json::Value& params) {
    JsResponse response;
    response.success = false;
    response.error = "Not implemented";
    return response;
}

JsResponse JsBridge::handleDeletePlaylist(const Json::Value& params) {
    JsResponse response;
    response.success = false;
    response.error = "Not implemented";
    return response;
}

JsResponse JsBridge::handleAddToPlaylist(const Json::Value& params) {
    JsResponse response;
    response.success = false;
    response.error = "Not implemented";
    return response;
}

JsResponse JsBridge::handleRemoveFromPlaylist(const Json::Value& params) {
    JsResponse response;
    response.success = false;
    response.error = "Not implemented";
    return response;
}

// Helper methods
Json::Value JsBridge::trackToJson(const TrackInfo& track) const {
    Json::Value obj;
    obj["id"] = track.id;
    obj["title"] = track.title;
    obj["artist"] = track.artist;
    obj["album"] = track.album;
    obj["genre"] = track.genre;
    obj["year"] = track.year;
    obj["trackNumber"] = track.trackNumber;
    obj["duration"] = static_cast<Json::Int64>(track.duration.count());
    obj["filePath"] = track.filePath.string();
    obj["coverUrl"] = track.coverUrl;
    obj["source"] = static_cast<int>(track.source);
    obj["sourceId"] = track.sourceId;
    return obj;
}

Json::Value JsBridge::playlistToJson(const PlaylistInfo& playlist) const {
    Json::Value obj;
    obj["id"] = playlist.id;
    obj["name"] = playlist.name;
    obj["description"] = playlist.description;
    obj["trackCount"] = static_cast<int>(playlist.trackIds.size());
    return obj;
}

Json::Value JsBridge::searchResultToJson(const SearchResult& result) const {
    Json::Value obj;
    obj["id"] = result.id;
    obj["title"] = result.title;
    obj["subtitle"] = result.subtitle;
    obj["thumbnailUrl"] = result.thumbnailUrl;
    obj["source"] = static_cast<int>(result.source);
    obj["sourceId"] = result.sourceId;
    obj["duration"] = static_cast<Json::Int64>(result.duration.count());
    obj["isPlaylist"] = result.isPlaylist;
    return obj;
}

} // namespace soda
