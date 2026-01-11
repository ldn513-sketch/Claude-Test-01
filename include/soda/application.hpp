/*
 * SODA Player - Application Core
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_APPLICATION_HPP
#define SODA_APPLICATION_HPP

#include "soda.hpp"
#include "audio_engine.hpp"
#include "config_manager.hpp"
#include "plugin_manager.hpp"
#include "source_manager.hpp"
#include "skin_manager.hpp"
#include "event_bus.hpp"

namespace soda {

class Application {
public:
    struct Options {
        Path configDir;
        Path dataDir;
        Path cacheDir;
        std::string skinName = "default-dark";
        bool headless = false;
        int logLevel = 1; // 0=error, 1=warn, 2=info, 3=debug
    };

    static Application& instance();

    // Lifecycle
    Result<void> initialize(const Options& options);
    void run();
    void shutdown();
    bool isRunning() const { return running_.load(); }

    // Component access
    AudioEngine& audioEngine() { return *audioEngine_; }
    ConfigManager& config() { return *configManager_; }
    PluginManager& plugins() { return *pluginManager_; }
    SourceManager& sources() { return *sourceManager_; }
    SkinManager& skins() { return *skinManager_; }
    EventBus& events() { return *eventBus_; }

    // Paths
    const Path& configDir() const { return options_.configDir; }
    const Path& dataDir() const { return options_.dataDir; }
    const Path& cacheDir() const { return options_.cacheDir; }
    Path musicDir() const { return options_.dataDir / "music"; }
    Path playlistDir() const { return options_.configDir / "playlists"; }
    Path skinsDir() const { return options_.dataDir / "skins"; }
    Path pluginsDir() const { return options_.dataDir / "plugins"; }

    // Quick actions
    void quit();
    void togglePlayPause();
    void playNext();
    void playPrevious();
    void setVolume(float volume);
    float getVolume() const;
    void seek(Duration position);

    // Search
    std::vector<SearchResult> search(const std::string& query, bool includeLocal = true,
                                     bool includeYouTube = true, bool includePodcasts = true);

private:
    Application() = default;
    ~Application() = default;
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void initializeDirectories();
    void loadConfiguration();
    void initializeComponents();
    void runMainLoop();

    Options options_;
    std::atomic<bool> running_{false};

    std::unique_ptr<EventBus> eventBus_;
    std::unique_ptr<ConfigManager> configManager_;
    std::unique_ptr<AudioEngine> audioEngine_;
    std::unique_ptr<PluginManager> pluginManager_;
    std::unique_ptr<SourceManager> sourceManager_;
    std::unique_ptr<SkinManager> skinManager_;
};

} // namespace soda

#endif // SODA_APPLICATION_HPP
