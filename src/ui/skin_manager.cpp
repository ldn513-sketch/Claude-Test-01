/*
 * SODA Player - Skin Manager Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/skin_manager.hpp>
#include <soda/event_bus.hpp>
#include <soda/config_manager.hpp>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>

namespace soda {

SkinManager::SkinManager(EventBus& eventBus, ConfigManager& config)
    : eventBus_(eventBus)
    , config_(config)
{
}

SkinManager::~SkinManager() = default;

Result<void> SkinManager::initialize(const Path& skinsDir) {
    skinsDir_ = skinsDir;
    std::filesystem::create_directories(skinsDir_);

    // Scan for available skins
    scanSkins();

    // Set initial skin from config
    std::string skinId = config_.settings().currentSkin;
    if (skins_.find(skinId) == skins_.end()) {
        skinId = "default-dark"; // Fallback
    }

    auto result = setSkin(skinId);
    if (!result.ok()) {
        // Try default-light as last resort
        if (skinId != "default-light") {
            return setSkin("default-light");
        }
        return result;
    }

    return Result<void>();
}

void SkinManager::scanSkins() {
    std::lock_guard<std::mutex> lock(mutex_);
    skins_.clear();

    try {
        for (const auto& entry : std::filesystem::directory_iterator(skinsDir_)) {
            if (!entry.is_directory()) continue;

            auto result = loadSkinInfo(entry.path());
            if (result.ok()) {
                skins_[result.value().id] = result.value();
            }
        }
    } catch (...) {
        // Ignore scan errors
    }
}

Result<SkinInfo> SkinManager::loadSkinInfo(const Path& skinDir) {
    Path manifestPath = skinDir / "manifest.yaml";
    if (!std::filesystem::exists(manifestPath)) {
        // Try to create minimal info from directory name
        SkinInfo info;
        info.id = skinDir.filename().string();
        info.name = info.id;
        info.path = skinDir;
        return Result<SkinInfo>(info);
    }

    try {
        YAML::Node manifest = YAML::LoadFile(manifestPath.string());

        SkinInfo info;
        info.id = manifest["id"].as<std::string>(skinDir.filename().string());
        info.name = manifest["name"].as<std::string>(info.id);
        info.version = manifest["version"].as<std::string>("1.0.0");
        info.author = manifest["author"].as<std::string>("");
        info.description = manifest["description"].as<std::string>("");
        info.path = skinDir;

        if (manifest["screenshot"]) {
            info.screenshot = manifest["screenshot"].as<std::string>();
        }

        if (manifest["tags"]) {
            for (const auto& tag : manifest["tags"]) {
                info.tags.push_back(tag.as<std::string>());
            }
        }

        return Result<SkinInfo>(info);

    } catch (const std::exception& e) {
        return Result<SkinInfo>("Failed to load skin manifest: " + std::string(e.what()));
    }
}

std::vector<SkinInfo> SkinManager::availableSkins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<SkinInfo> result;
    result.reserve(skins_.size());
    for (const auto& [id, skin] : skins_) {
        result.push_back(skin);
    }
    return result;
}

std::optional<SkinInfo> SkinManager::getSkinInfo(const std::string& skinId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = skins_.find(skinId);
    if (it != skins_.end()) {
        return it->second;
    }
    return std::nullopt;
}

Result<void> SkinManager::setSkin(const std::string& skinId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = skins_.find(skinId);
    if (it == skins_.end()) {
        return Result<void>("Skin not found: " + skinId);
    }

    currentSkinId_ = skinId;
    const Path& skinPath = it->second.path;

    // Load skin files
    try {
        // Load HTML
        Path htmlPath = skinPath / "index.html";
        if (std::filesystem::exists(htmlPath)) {
            std::ifstream file(htmlPath);
            std::stringstream buffer;
            buffer << file.rdbuf();
            cachedHtml_ = buffer.str();
        } else {
            cachedHtml_ = "<html><body><h1>SODA Player</h1></body></html>";
        }

        // Load CSS
        Path cssPath = skinPath / "style.css";
        if (std::filesystem::exists(cssPath)) {
            std::ifstream file(cssPath);
            std::stringstream buffer;
            buffer << file.rdbuf();
            cachedCss_ = buffer.str();
        } else {
            cachedCss_ = "";
        }

        // Load JS
        Path jsPath = skinPath / "script.js";
        if (std::filesystem::exists(jsPath)) {
            std::ifstream file(jsPath);
            std::stringstream buffer;
            buffer << file.rdbuf();
            cachedJs_ = buffer.str();
        } else {
            cachedJs_ = "";
        }

    } catch (const std::exception& e) {
        return Result<void>("Failed to load skin: " + std::string(e.what()));
    }

    return Result<void>();
}

void SkinManager::reloadCurrentSkin() {
    setSkin(currentSkinId_);
}

std::string SkinManager::getHtml() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cachedHtml_;
}

std::string SkinManager::getCss() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cachedCss_;
}

std::string SkinManager::getJs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cachedJs_;
}

std::string SkinManager::getFullPage() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string apiBindings = generateApiBindings();

    return wrapInHtmlDocument(cachedHtml_, cachedCss_, apiBindings + cachedJs_);
}

std::string SkinManager::generateApiBindings() const {
    return R"(
// SODA Player JavaScript API
window.soda = {
    _callbacks: {},
    _nextCallbackId: 1,

    _call: function(method, params, callback) {
        var callbackId = null;
        if (callback) {
            callbackId = 'cb_' + this._nextCallbackId++;
            this._callbacks[callbackId] = callback;
        }

        var message = JSON.stringify({
            method: method,
            params: params || {},
            callbackId: callbackId
        });

        window.webkit.messageHandlers.soda.postMessage(message);
    },

    _handleResponse: function(responseJson) {
        var response = JSON.parse(responseJson);
        if (response.callbackId && this._callbacks[response.callbackId]) {
            this._callbacks[response.callbackId](response.success, response.result, response.error);
            delete this._callbacks[response.callbackId];
        }
    },

    _handleEvent: function(eventJson) {
        var event = JSON.parse(eventJson);
        if (this.onEvent) {
            this.onEvent(event);
        }
    },

    // Playback control
    play: function(cb) { this._call('play', {}, cb); },
    pause: function(cb) { this._call('pause', {}, cb); },
    stop: function(cb) { this._call('stop', {}, cb); },
    togglePlayPause: function(cb) { this._call('togglePlayPause', {}, cb); },
    next: function(cb) { this._call('next', {}, cb); },
    previous: function(cb) { this._call('previous', {}, cb); },
    seek: function(position, cb) { this._call('seek', {position: position}, cb); },

    // Volume
    setVolume: function(volume, cb) { this._call('setVolume', {volume: volume}, cb); },
    getVolume: function(cb) { this._call('getVolume', {}, cb); },

    // State
    getState: function(cb) { this._call('getState', {}, cb); },
    getCurrentTrack: function(cb) { this._call('getCurrentTrack', {}, cb); },
    getPosition: function(cb) { this._call('getPosition', {}, cb); },

    // Queue
    queueAdd: function(trackId, cb) { this._call('queueAdd', {trackId: trackId}, cb); },
    queueRemove: function(index, cb) { this._call('queueRemove', {index: index}, cb); },
    queueClear: function(cb) { this._call('queueClear', {}, cb); },
    queueGet: function(cb) { this._call('queueGet', {}, cb); },
    queueJumpTo: function(index, cb) { this._call('queueJumpTo', {index: index}, cb); },
    queueShuffle: function(cb) { this._call('queueShuffle', {}, cb); },

    // Library
    search: function(query, cb) { this._call('search', {query: query}, cb); },
    getTracks: function(cb) { this._call('getTracks', {}, cb); },
    getArtists: function(cb) { this._call('getArtists', {}, cb); },
    getAlbums: function(cb) { this._call('getAlbums', {}, cb); },
    getTracksByArtist: function(artist, cb) { this._call('getTracksByArtist', {artist: artist}, cb); },
    getTracksByAlbum: function(album, cb) { this._call('getTracksByAlbum', {album: album}, cb); },

    // Playlists
    getPlaylists: function(cb) { this._call('getPlaylists', {}, cb); },
    getPlaylist: function(id, cb) { this._call('getPlaylist', {id: id}, cb); },
    createPlaylist: function(name, cb) { this._call('createPlaylist', {name: name}, cb); },
    deletePlaylist: function(id, cb) { this._call('deletePlaylist', {id: id}, cb); },
    addToPlaylist: function(playlistId, trackId, cb) {
        this._call('addToPlaylist', {playlistId: playlistId, trackId: trackId}, cb);
    },

    // Settings
    getSettings: function(cb) { this._call('getSettings', {}, cb); },
    setSettings: function(settings, cb) { this._call('setSettings', {settings: settings}, cb); },
    getSkins: function(cb) { this._call('getSkins', {}, cb); },
    setSkin: function(skinId, cb) { this._call('setSkin', {skinId: skinId}, cb); },

    // Window
    minimizeWindow: function(cb) { this._call('minimizeWindow', {}, cb); },
    maximizeWindow: function(cb) { this._call('maximizeWindow', {}, cb); },
    closeWindow: function(cb) { this._call('closeWindow', {}, cb); },
    quit: function(cb) { this._call('quit', {}, cb); },

    // Event handler (override this in your skin)
    onEvent: null
};

console.log('SODA API initialized');
)";
}

std::string SkinManager::wrapInHtmlDocument(const std::string& bodyContent,
                                             const std::string& css,
                                             const std::string& js) const {
    std::stringstream html;

    html << R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SODA Player</title>
    <style>
)" << css << R"(
    </style>
</head>
<body>
)" << bodyContent << R"(
    <script>
)" << js << R"(
    </script>
</body>
</html>)";

    return html.str();
}

Result<void> SkinManager::installSkin(const Path& archivePath) {
    // TODO: Extract archive and install
    return Result<void>("Skin installation not yet implemented");
}

Result<void> SkinManager::uninstallSkin(const std::string& skinId) {
    if (skinId == "default-dark" || skinId == "default-light") {
        return Result<void>("Cannot uninstall built-in skins");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = skins_.find(skinId);
    if (it == skins_.end()) {
        return Result<void>("Skin not found");
    }

    std::filesystem::remove_all(it->second.path);
    skins_.erase(it);

    return Result<void>();
}

Result<void> SkinManager::createSkin(const std::string& name, const std::string& baseTheme) {
    std::string skinId = name;
    std::transform(skinId.begin(), skinId.end(), skinId.begin(), ::tolower);
    std::replace(skinId.begin(), skinId.end(), ' ', '-');

    Path skinPath = skinsDir_ / skinId;
    if (std::filesystem::exists(skinPath)) {
        return Result<void>("Skin already exists");
    }

    std::filesystem::create_directories(skinPath);

    // Copy base theme if specified
    auto baseIt = skins_.find(baseTheme);
    if (baseIt != skins_.end()) {
        std::filesystem::copy(baseIt->second.path, skinPath,
                             std::filesystem::copy_options::recursive);
    }

    // Create manifest
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "id" << YAML::Value << skinId;
    out << YAML::Key << "name" << YAML::Value << name;
    out << YAML::Key << "version" << YAML::Value << "1.0.0";
    out << YAML::Key << "author" << YAML::Value << "User";
    out << YAML::Key << "description" << YAML::Value << "Custom skin";
    out << YAML::EndMap;

    std::ofstream manifest(skinPath / "manifest.yaml");
    manifest << out.c_str();

    // Re-scan to pick up new skin
    scanSkins();

    return Result<void>();
}

bool SkinManager::validateSkin(const Path& skinDir) const {
    // Check for required files
    return std::filesystem::exists(skinDir / "index.html") ||
           std::filesystem::exists(skinDir / "manifest.yaml");
}

} // namespace soda
