/*
 * SODA Player - Configuration Manager Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/config_manager.hpp>
#include <fstream>
#include <iostream>

namespace soda {

ConfigManager::ConfigManager(const Path& configDir)
    : configDir_(configDir)
    , configFile_(configDir / "config.yaml")
{
}

Result<void> ConfigManager::initialize() {
    std::filesystem::create_directories(configDir_);

    if (std::filesystem::exists(configFile_)) {
        return loadConfig();
    } else {
        createDefaultConfig();
        return save();
    }
}

void ConfigManager::createDefaultConfig() {
    settings_ = AppSettings{};

    // Set default music folder
    const char* home = std::getenv("HOME");
    if (home) {
        Path musicPath = Path(home) / "Music";
        if (std::filesystem::exists(musicPath)) {
            settings_.musicFolders.push_back(musicPath);
        }

        settings_.downloadFolder = Path(home) / ".local" / "share" / "soda-player" / "music";
    }

    configNode_ = YAML::Node();
    configNode_["version"] = configVersion_;
    dirty_ = true;
}

Result<void> ConfigManager::loadConfig() {
    try {
        configNode_ = YAML::LoadFile(configFile_.string());

        // Check version and migrate if needed
        int fileVersion = configNode_["version"].as<int>(1);
        if (fileVersion < configVersion_) {
            migrateIfNeeded(fileVersion);
        }

        // Load settings
        if (configNode_["audio"]) {
            auto audio = configNode_["audio"];
            settings_.volume = audio["volume"].as<float>(0.8f);
            settings_.audioDevice = audio["device"].as<std::string>("default");
            settings_.bufferSize = audio["bufferSize"].as<int>(1024);
            settings_.normalizeVolume = audio["normalize"].as<bool>(false);
        }

        if (configNode_["playback"]) {
            auto playback = configNode_["playback"];
            settings_.repeatMode = playback["repeat"].as<RepeatMode>(RepeatMode::Off);
            settings_.shuffle = playback["shuffle"].as<bool>(false);
            settings_.saveQueueOnExit = playback["saveQueue"].as<bool>(true);
            settings_.resumeOnStart = playback["resume"].as<bool>(true);
        }

        if (configNode_["library"]) {
            auto library = configNode_["library"];
            if (library["folders"]) {
                for (const auto& folder : library["folders"]) {
                    settings_.musicFolders.push_back(folder.as<std::string>());
                }
            }
            settings_.watchFolders = library["watch"].as<bool>(true);
            settings_.scanInterval = library["scanInterval"].as<int>(300);
        }

        if (configNode_["cache"]) {
            auto cache = configNode_["cache"];
            settings_.maxCacheSize = cache["maxSize"].as<size_t>(1024 * 1024 * 1024);
            settings_.autoCleanCache = cache["autoClean"].as<bool>(true);
            settings_.cacheExpiryDays = cache["expiryDays"].as<int>(30);
        }

        if (configNode_["downloads"]) {
            auto downloads = configNode_["downloads"];
            settings_.downloadFolder = downloads["folder"].as<std::string>("");
            settings_.preferredFormat = downloads["format"].as<std::string>("opus");
            settings_.preferredQuality = downloads["quality"].as<std::string>("high");
            settings_.organizeDownloads = downloads["organize"].as<bool>(true);
        }

        if (configNode_["youtube"]) {
            auto youtube = configNode_["youtube"];
            settings_.youtubeEnabled = youtube["enabled"].as<bool>(true);
            settings_.autoUpdatePlaylists = youtube["autoUpdate"].as<bool>(false);
            settings_.playlistCheckInterval = youtube["checkInterval"].as<int>(3600);
        }

        if (configNode_["podcasts"]) {
            auto podcasts = configNode_["podcasts"];
            settings_.podcastsEnabled = podcasts["enabled"].as<bool>(true);
            settings_.autoDownloadEpisodes = podcasts["autoDownload"].as<bool>(false);
            settings_.episodeRetentionDays = podcasts["retentionDays"].as<int>(30);
        }

        if (configNode_["interface"]) {
            auto ui = configNode_["interface"];
            settings_.currentSkin = ui["skin"].as<std::string>("default-dark");
            settings_.language = ui["language"].as<std::string>("en");
            settings_.showNotifications = ui["notifications"].as<bool>(true);
            settings_.minimizeToTray = ui["minimizeToTray"].as<bool>(false);
        }

        if (configNode_["privacy"]) {
            auto privacy = configNode_["privacy"];
            settings_.enableScrobbling = privacy["scrobbling"].as<bool>(false);
            settings_.lastfmUsername = privacy["lastfmUser"].as<std::string>("");
            settings_.lastfmSessionKey = privacy["lastfmSession"].as<std::string>("");
        }

        if (configNode_["advanced"]) {
            auto advanced = configNode_["advanced"];
            settings_.logLevel = advanced["logLevel"].as<int>(1);
            settings_.hardwareAcceleration = advanced["hwAccel"].as<bool>(true);
            settings_.httpTimeout = advanced["httpTimeout"].as<int>(30);
        }

        dirty_ = false;
        return Result<void>();

    } catch (const std::exception& e) {
        return Result<void>("Failed to load configuration: " + std::string(e.what()));
    }
}

Result<void> ConfigManager::save() {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        YAML::Emitter out;
        out << YAML::BeginMap;

        out << YAML::Key << "version" << YAML::Value << configVersion_;

        // Audio settings
        out << YAML::Key << "audio" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "volume" << YAML::Value << settings_.volume;
        out << YAML::Key << "device" << YAML::Value << settings_.audioDevice;
        out << YAML::Key << "bufferSize" << YAML::Value << settings_.bufferSize;
        out << YAML::Key << "normalize" << YAML::Value << settings_.normalizeVolume;
        out << YAML::EndMap;

        // Playback settings
        out << YAML::Key << "playback" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "repeat" << YAML::Value << settings_.repeatMode;
        out << YAML::Key << "shuffle" << YAML::Value << settings_.shuffle;
        out << YAML::Key << "saveQueue" << YAML::Value << settings_.saveQueueOnExit;
        out << YAML::Key << "resume" << YAML::Value << settings_.resumeOnStart;
        out << YAML::EndMap;

        // Library settings
        out << YAML::Key << "library" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "folders" << YAML::Value << YAML::BeginSeq;
        for (const auto& folder : settings_.musicFolders) {
            out << folder.string();
        }
        out << YAML::EndSeq;
        out << YAML::Key << "watch" << YAML::Value << settings_.watchFolders;
        out << YAML::Key << "scanInterval" << YAML::Value << settings_.scanInterval;
        out << YAML::EndMap;

        // Cache settings
        out << YAML::Key << "cache" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "maxSize" << YAML::Value << settings_.maxCacheSize;
        out << YAML::Key << "autoClean" << YAML::Value << settings_.autoCleanCache;
        out << YAML::Key << "expiryDays" << YAML::Value << settings_.cacheExpiryDays;
        out << YAML::EndMap;

        // Download settings
        out << YAML::Key << "downloads" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "folder" << YAML::Value << settings_.downloadFolder.string();
        out << YAML::Key << "format" << YAML::Value << settings_.preferredFormat;
        out << YAML::Key << "quality" << YAML::Value << settings_.preferredQuality;
        out << YAML::Key << "organize" << YAML::Value << settings_.organizeDownloads;
        out << YAML::EndMap;

        // YouTube settings
        out << YAML::Key << "youtube" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "enabled" << YAML::Value << settings_.youtubeEnabled;
        out << YAML::Key << "autoUpdate" << YAML::Value << settings_.autoUpdatePlaylists;
        out << YAML::Key << "checkInterval" << YAML::Value << settings_.playlistCheckInterval;
        out << YAML::EndMap;

        // Podcast settings
        out << YAML::Key << "podcasts" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "enabled" << YAML::Value << settings_.podcastsEnabled;
        out << YAML::Key << "autoDownload" << YAML::Value << settings_.autoDownloadEpisodes;
        out << YAML::Key << "retentionDays" << YAML::Value << settings_.episodeRetentionDays;
        out << YAML::EndMap;

        // Interface settings
        out << YAML::Key << "interface" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "skin" << YAML::Value << settings_.currentSkin;
        out << YAML::Key << "language" << YAML::Value << settings_.language;
        out << YAML::Key << "notifications" << YAML::Value << settings_.showNotifications;
        out << YAML::Key << "minimizeToTray" << YAML::Value << settings_.minimizeToTray;
        out << YAML::EndMap;

        // Privacy settings
        out << YAML::Key << "privacy" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "scrobbling" << YAML::Value << settings_.enableScrobbling;
        out << YAML::Key << "lastfmUser" << YAML::Value << settings_.lastfmUsername;
        out << YAML::Key << "lastfmSession" << YAML::Value << settings_.lastfmSessionKey;
        out << YAML::EndMap;

        // Advanced settings
        out << YAML::Key << "advanced" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "logLevel" << YAML::Value << settings_.logLevel;
        out << YAML::Key << "hwAccel" << YAML::Value << settings_.hardwareAcceleration;
        out << YAML::Key << "httpTimeout" << YAML::Value << settings_.httpTimeout;
        out << YAML::EndMap;

        out << YAML::EndMap;

        std::ofstream file(configFile_);
        if (!file) {
            return Result<void>("Failed to open config file for writing");
        }

        file << out.c_str();
        dirty_ = false;
        return Result<void>();

    } catch (const std::exception& e) {
        return Result<void>("Failed to save configuration: " + std::string(e.what()));
    }
}

void ConfigManager::migrateIfNeeded(int fromVersion) {
    // Handle config file migrations between versions
    // Currently only version 1, so nothing to migrate
    configNode_["version"] = configVersion_;
    dirty_ = true;
}

Result<void> ConfigManager::backup(const Path& backupPath) {
    try {
        std::filesystem::copy_file(configFile_, backupPath,
                                   std::filesystem::copy_options::overwrite_existing);
        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>("Backup failed: " + std::string(e.what()));
    }
}

Result<void> ConfigManager::restore(const Path& backupPath) {
    if (!std::filesystem::exists(backupPath)) {
        return Result<void>("Backup file not found");
    }

    try {
        std::filesystem::copy_file(backupPath, configFile_,
                                   std::filesystem::copy_options::overwrite_existing);
        return loadConfig();
    } catch (const std::exception& e) {
        return Result<void>("Restore failed: " + std::string(e.what()));
    }
}

Result<void> ConfigManager::exportConfig(const Path& exportPath) {
    return backup(exportPath);
}

Result<void> ConfigManager::importConfig(const Path& importPath) {
    return restore(importPath);
}

void ConfigManager::resetToDefaults() {
    createDefaultConfig();
}

} // namespace soda
