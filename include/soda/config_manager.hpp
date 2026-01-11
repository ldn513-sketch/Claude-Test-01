/*
 * SODA Player - Configuration Manager
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_CONFIG_MANAGER_HPP
#define SODA_CONFIG_MANAGER_HPP

#include "soda.hpp"
#include <yaml-cpp/yaml.h>

namespace soda {

// Settings structure with all configurable options
struct AppSettings {
    // Audio
    float volume = 0.8f;
    std::string audioDevice = "default";
    int bufferSize = 1024;
    bool normalizeVolume = false;

    // Playback
    RepeatMode repeatMode = RepeatMode::Off;
    bool shuffle = false;
    bool saveQueueOnExit = true;
    bool resumeOnStart = true;

    // Library
    std::vector<Path> musicFolders;
    bool watchFolders = true;
    int scanInterval = 300; // seconds

    // Cache
    size_t maxCacheSize = 1024 * 1024 * 1024; // 1GB
    bool autoCleanCache = true;
    int cacheExpiryDays = 30;

    // Downloads
    Path downloadFolder;
    std::string preferredFormat = "opus";
    std::string preferredQuality = "high";
    bool organizeDownloads = true; // Artist/Album/Track structure

    // YouTube
    bool youtubeEnabled = true;
    bool autoUpdatePlaylists = false;
    int playlistCheckInterval = 3600; // seconds

    // Podcasts
    bool podcastsEnabled = true;
    bool autoDownloadEpisodes = false;
    int episodeRetentionDays = 30;

    // Interface
    std::string currentSkin = "default-dark";
    std::string language = "en";
    bool showNotifications = true;
    bool minimizeToTray = false;

    // Privacy
    bool enableScrobbling = false;
    std::string lastfmUsername;
    std::string lastfmSessionKey;

    // Advanced
    int logLevel = 1;
    bool hardwareAcceleration = true;
    int httpTimeout = 30; // seconds
};

class ConfigManager {
public:
    explicit ConfigManager(const Path& configDir);
    ~ConfigManager() = default;

    // Initialize (load or create default config)
    Result<void> initialize();

    // Save configuration
    Result<void> save();

    // Settings access
    AppSettings& settings() { return settings_; }
    const AppSettings& settings() const { return settings_; }

    // Individual setting access with type safety
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T{}) const;

    template<typename T>
    void set(const std::string& key, const T& value);

    // Check if config has been modified
    bool isDirty() const { return dirty_; }

    // Backup and restore
    Result<void> backup(const Path& backupPath);
    Result<void> restore(const Path& backupPath);

    // Export/Import
    Result<void> exportConfig(const Path& exportPath);
    Result<void> importConfig(const Path& importPath);

    // Reset to defaults
    void resetToDefaults();

private:
    void createDefaultConfig();
    Result<void> loadConfig();
    void migrateIfNeeded(int fromVersion);

    Path configDir_;
    Path configFile_;
    AppSettings settings_;
    YAML::Node configNode_;
    bool dirty_ = false;
    int configVersion_ = 1;

    mutable std::mutex mutex_;
};

// Template implementations
template<typename T>
T ConfigManager::get(const std::string& key, const T& defaultValue) const {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        if (configNode_[key]) {
            return configNode_[key].as<T>();
        }
    } catch (...) {}
    return defaultValue;
}

template<typename T>
void ConfigManager::set(const std::string& key, const T& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    configNode_[key] = value;
    dirty_ = true;
}

} // namespace soda

// YAML conversion for custom types
namespace YAML {

template<>
struct convert<soda::RepeatMode> {
    static Node encode(const soda::RepeatMode& rhs) {
        switch (rhs) {
            case soda::RepeatMode::Off: return Node("off");
            case soda::RepeatMode::One: return Node("one");
            case soda::RepeatMode::All: return Node("all");
            default: return Node("off");
        }
    }

    static bool decode(const Node& node, soda::RepeatMode& rhs) {
        std::string value = node.as<std::string>();
        if (value == "off") rhs = soda::RepeatMode::Off;
        else if (value == "one") rhs = soda::RepeatMode::One;
        else if (value == "all") rhs = soda::RepeatMode::All;
        else return false;
        return true;
    }
};

template<>
struct convert<std::filesystem::path> {
    static Node encode(const std::filesystem::path& rhs) {
        return Node(rhs.string());
    }

    static bool decode(const Node& node, std::filesystem::path& rhs) {
        rhs = std::filesystem::path(node.as<std::string>());
        return true;
    }
};

} // namespace YAML

#endif // SODA_CONFIG_MANAGER_HPP
