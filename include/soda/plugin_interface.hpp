/*
 * SODA Player - Plugin Interface
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 *
 * This header defines the interface that all plugins must implement.
 * Plugins are shared libraries (.so on Linux) that export a create function.
 */

#ifndef SODA_PLUGIN_INTERFACE_HPP
#define SODA_PLUGIN_INTERFACE_HPP

#include "soda.hpp"

namespace soda {

// Forward declaration
class Application;

// Plugin API version for compatibility checking
#define SODA_PLUGIN_API_VERSION 1

// Plugin categories
enum class PluginCategory {
    Audio,      // Equalizers, effects, codecs
    Visual,     // Video player, visualizers
    Tools,      // OCR, translation, library management
    Social,     // Last.fm, sharing
    Source,     // Additional sources (SoundCloud, etc.)
    Other
};

// Plugin interface that all plugins must implement
class PluginInterface {
public:
    virtual ~PluginInterface() = default;

    // Plugin metadata
    virtual std::string id() const = 0;
    virtual std::string name() const = 0;
    virtual std::string version() const = 0;
    virtual std::string author() const = 0;
    virtual std::string description() const = 0;
    virtual PluginCategory category() const = 0;
    virtual int apiVersion() const { return SODA_PLUGIN_API_VERSION; }

    // Lifecycle
    virtual Result<void> initialize(Application& app) = 0;
    virtual void shutdown() = 0;

    // Called when plugin is enabled/disabled
    virtual void onEnable() {}
    virtual void onDisable() {}

    // Event handling
    virtual void onEvent(const Event& event) {}

    // Configuration
    virtual bool hasSettings() const { return false; }
    virtual std::string settingsHtml() const { return ""; } // HTML for settings UI
    virtual void applySettings(const std::map<std::string, std::string>& settings) {}

    // Required permissions
    virtual std::vector<std::string> requiredPermissions() const { return {}; }
};

// Audio processor plugin interface (for equalizers, effects, etc.)
class AudioProcessorPlugin : public PluginInterface {
public:
    PluginCategory category() const override { return PluginCategory::Audio; }

    // Process audio frames (in-place modification)
    virtual void process(float* samples, size_t frameCount, int channels, int sampleRate) = 0;

    // Get/Set parameters
    virtual std::vector<std::string> parameterNames() const { return {}; }
    virtual float getParameter(const std::string& name) const { return 0.0f; }
    virtual void setParameter(const std::string& name, float value) {}
};

// Visualizer plugin interface
class VisualizerPlugin : public PluginInterface {
public:
    PluginCategory category() const override { return PluginCategory::Visual; }

    // Receive audio data for visualization
    virtual void pushAudioData(const float* samples, size_t frameCount, int channels) = 0;

    // Render visualization to HTML canvas or get render data
    virtual std::string renderHtml() const { return ""; }
};

// Source plugin interface (for additional streaming sources)
class SourcePlugin : public PluginInterface {
public:
    PluginCategory category() const override { return PluginCategory::Source; }

    // Source identification
    virtual std::string sourceId() const = 0;
    virtual std::string sourceName() const = 0;
    virtual std::string sourceIcon() const { return ""; } // SVG or base64 icon

    // Search
    virtual std::vector<SearchResult> search(const std::string& query) = 0;

    // Get track/stream URL
    virtual Result<std::string> getStreamUrl(const std::string& trackId) = 0;
    virtual Result<TrackInfo> getTrackInfo(const std::string& trackId) = 0;

    // Playlist support
    virtual bool supportsPlaylists() const { return false; }
    virtual std::vector<PlaylistInfo> getUserPlaylists() { return {}; }
    virtual std::vector<TrackInfo> getPlaylistTracks(const std::string& playlistId) { return {}; }
};

// Scrobbler plugin interface (Last.fm, Libre.fm, etc.)
class ScrobblerPlugin : public PluginInterface {
public:
    PluginCategory category() const override { return PluginCategory::Social; }

    // Authentication
    virtual bool isAuthenticated() const = 0;
    virtual Result<std::string> getAuthUrl() = 0;
    virtual Result<void> authenticate(const std::string& token) = 0;
    virtual void logout() = 0;

    // Scrobbling
    virtual void nowPlaying(const TrackInfo& track) = 0;
    virtual void scrobble(const TrackInfo& track, Duration listenedTime) = 0;

    // Love/Unlove
    virtual void love(const TrackInfo& track) = 0;
    virtual void unlove(const TrackInfo& track) = 0;
};

} // namespace soda

// Macro for plugin export (use in plugin implementation)
#define SODA_PLUGIN_EXPORT(PluginClass)                                          \
    extern "C" {                                                                  \
        soda::PluginInterface* soda_plugin_create() {                            \
            return new PluginClass();                                             \
        }                                                                         \
        void soda_plugin_destroy(soda::PluginInterface* plugin) {                \
            delete plugin;                                                        \
        }                                                                         \
        int soda_plugin_api_version() {                                          \
            return SODA_PLUGIN_API_VERSION;                                       \
        }                                                                         \
    }

#endif // SODA_PLUGIN_INTERFACE_HPP
