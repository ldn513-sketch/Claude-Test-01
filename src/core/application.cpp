/*
 * SODA Player - Application Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/application.hpp>
#include <soda/webview_window.hpp>
#include <iostream>
#include <filesystem>

namespace soda {

Application& Application::instance() {
    static Application instance;
    return instance;
}

Result<void> Application::initialize(const Options& options) {
    options_ = options;

    try {
        // Create directories
        initializeDirectories();

        // Initialize event bus first (other components depend on it)
        eventBus_ = std::make_unique<EventBus>();

        // Load configuration
        configManager_ = std::make_unique<ConfigManager>(options_.configDir);
        auto configResult = configManager_->initialize();
        if (!configResult.ok()) {
            return Result<void>("Failed to load configuration: " + configResult.error());
        }

        // Initialize audio engine
        audioEngine_ = std::make_unique<AudioEngine>(*eventBus_);
        auto audioResult = audioEngine_->initialize();
        if (!audioResult.ok()) {
            return Result<void>("Failed to initialize audio engine: " + audioResult.error());
        }

        // Initialize plugin manager
        pluginManager_ = std::make_unique<PluginManager>(*this);
        auto pluginResult = pluginManager_->initialize(pluginsDir());
        if (!pluginResult.ok()) {
            // Plugin init failure is not fatal, just log it
            std::cerr << "Warning: Plugin system initialization failed: " << pluginResult.error() << "\n";
        }

        // Initialize source manager
        sourceManager_ = std::make_unique<SourceManager>(*eventBus_, *configManager_);
        auto sourceResult = sourceManager_->initialize(options_.dataDir);
        if (!sourceResult.ok()) {
            return Result<void>("Failed to initialize source manager: " + sourceResult.error());
        }

        // Initialize skin manager
        skinManager_ = std::make_unique<SkinManager>(*eventBus_, *configManager_);
        auto skinResult = skinManager_->initialize(skinsDir());
        if (!skinResult.ok()) {
            // Skin init failure is not fatal if running headless
            if (!options_.headless) {
                return Result<void>("Failed to initialize skin manager: " + skinResult.error());
            }
        }

        // Set current skin
        if (!options_.headless && !options_.skinName.empty()) {
            skinManager_->setSkin(options_.skinName);
        }

        running_.store(true);
        return Result<void>();

    } catch (const std::exception& e) {
        return Result<void>(std::string("Initialization error: ") + e.what());
    }
}

void Application::initializeDirectories() {
    std::filesystem::create_directories(options_.configDir);
    std::filesystem::create_directories(options_.dataDir);
    std::filesystem::create_directories(options_.cacheDir);
    std::filesystem::create_directories(musicDir());
    std::filesystem::create_directories(playlistDir());
    std::filesystem::create_directories(skinsDir());
    std::filesystem::create_directories(pluginsDir());
}

void Application::run() {
    if (!running_.load()) {
        return;
    }

    if (options_.headless) {
        // Headless mode - just process events in a loop
        while (running_.load()) {
            eventBus_->processQueue();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } else {
        // GUI mode - create and run window
        WebViewWindow window(*eventBus_, *skinManager_);
        auto result = window.initialize();
        if (!result.ok()) {
            std::cerr << "Failed to create window: " << result.error() << "\n";
            return;
        }

        // Subscribe to quit event
        eventBus_->subscribe(EventType::Error, [this](const Event& event) {
            if (auto* msg = std::get_if<std::string>(&event.data)) {
                std::cerr << "Error: " << *msg << "\n";
            }
        });

        window.run();
    }
}

void Application::shutdown() {
    running_.store(false);

    // Save configuration
    if (configManager_) {
        configManager_->save();
    }

    // Shutdown components in reverse order
    if (skinManager_) {
        skinManager_.reset();
    }
    if (sourceManager_) {
        sourceManager_->shutdown();
        sourceManager_.reset();
    }
    if (pluginManager_) {
        pluginManager_->shutdown();
        pluginManager_.reset();
    }
    if (audioEngine_) {
        audioEngine_->shutdown();
        audioEngine_.reset();
    }
    if (configManager_) {
        configManager_.reset();
    }
    if (eventBus_) {
        eventBus_.reset();
    }
}

void Application::quit() {
    running_.store(false);
}

void Application::togglePlayPause() {
    if (audioEngine_) {
        audioEngine_->togglePlayPause();
    }
}

void Application::playNext() {
    if (audioEngine_) {
        audioEngine_->playNext();
    }
}

void Application::playPrevious() {
    if (audioEngine_) {
        audioEngine_->playPrevious();
    }
}

void Application::setVolume(float volume) {
    if (audioEngine_) {
        audioEngine_->setVolume(volume);
    }
}

float Application::getVolume() const {
    if (audioEngine_) {
        return audioEngine_->volume();
    }
    return 0.0f;
}

void Application::seek(Duration position) {
    if (audioEngine_) {
        audioEngine_->seek(position);
    }
}

std::vector<SearchResult> Application::search(const std::string& query,
                                               bool includeLocal,
                                               bool includeYouTube,
                                               bool includePodcasts) {
    if (sourceManager_) {
        return sourceManager_->search(query, includeLocal, includeYouTube, includePodcasts);
    }
    return {};
}

} // namespace soda
