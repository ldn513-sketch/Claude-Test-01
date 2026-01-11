/*
 * SODA Player - Main Entry Point
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/application.hpp>
#include <iostream>
#include <cstring>
#include <csignal>

namespace {
    volatile sig_atomic_t g_running = 1;

    void signalHandler(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            g_running = 0;
            soda::Application::instance().quit();
        }
    }

    void printVersion() {
        std::cout << "SODA Player version " << SODA_VERSION_STRING << "\n"
                  << "Copyright (C) 2026 SODA Project Contributors\n"
                  << "License: GPL-3.0-or-later\n"
                  << "This is free software: you are free to change and redistribute it.\n"
                  << "There is NO WARRANTY, to the extent permitted by law.\n";
    }

    void printHelp(const char* program) {
        std::cout << "Usage: " << program << " [OPTIONS] [FILE...]\n\n"
                  << "SODA Player - Streaming and Offline Digital Audio Player\n\n"
                  << "Options:\n"
                  << "  -h, --help              Show this help message\n"
                  << "  -v, --version           Show version information\n"
                  << "  --headless              Run without GUI\n"
                  << "  --config-dir DIR        Set configuration directory\n"
                  << "  --data-dir DIR          Set data directory\n"
                  << "  --cache-dir DIR         Set cache directory\n"
                  << "  --skin NAME             Set skin (default: default-dark)\n"
                  << "  --debug                 Enable debug logging\n"
                  << "\n"
                  << "Examples:\n"
                  << "  " << program << "                     Start SODA Player\n"
                  << "  " << program << " music.mp3           Play a file\n"
                  << "  " << program << " --skin default-light  Start with light theme\n"
                  << "\n"
                  << "Report bugs at: https://github.com/soda-player/soda-player/issues\n";
    }
}

int main(int argc, char* argv[]) {
    // Set up signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Parse command line arguments
    soda::Application::Options options;
    std::vector<std::string> filesToPlay;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            return 0;
        }
        if (arg == "-v" || arg == "--version") {
            printVersion();
            return 0;
        }
        if (arg == "--headless") {
            options.headless = true;
            continue;
        }
        if (arg == "--debug") {
            options.logLevel = 3;
            continue;
        }
        if (arg == "--config-dir" && i + 1 < argc) {
            options.configDir = argv[++i];
            continue;
        }
        if (arg == "--data-dir" && i + 1 < argc) {
            options.dataDir = argv[++i];
            continue;
        }
        if (arg == "--cache-dir" && i + 1 < argc) {
            options.cacheDir = argv[++i];
            continue;
        }
        if (arg == "--skin" && i + 1 < argc) {
            options.skinName = argv[++i];
            continue;
        }

        // Unknown option
        if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            std::cerr << "Try '" << argv[0] << " --help' for more information.\n";
            return 1;
        }

        // Assume it's a file to play
        filesToPlay.push_back(arg);
    }

    // Set default directories if not specified
    if (options.configDir.empty()) {
        const char* xdgConfig = std::getenv("XDG_CONFIG_HOME");
        if (xdgConfig) {
            options.configDir = soda::Path(xdgConfig) / "soda-player";
        } else {
            const char* home = std::getenv("HOME");
            if (home) {
                options.configDir = soda::Path(home) / ".config" / "soda-player";
            } else {
                options.configDir = ".soda-player";
            }
        }
    }

    if (options.dataDir.empty()) {
        const char* xdgData = std::getenv("XDG_DATA_HOME");
        if (xdgData) {
            options.dataDir = soda::Path(xdgData) / "soda-player";
        } else {
            const char* home = std::getenv("HOME");
            if (home) {
                options.dataDir = soda::Path(home) / ".local" / "share" / "soda-player";
            } else {
                options.dataDir = ".soda-player-data";
            }
        }
    }

    if (options.cacheDir.empty()) {
        const char* xdgCache = std::getenv("XDG_CACHE_HOME");
        if (xdgCache) {
            options.cacheDir = soda::Path(xdgCache) / "soda-player";
        } else {
            const char* home = std::getenv("HOME");
            if (home) {
                options.cacheDir = soda::Path(home) / ".cache" / "soda-player";
            } else {
                options.cacheDir = ".soda-player-cache";
            }
        }
    }

    // Initialize application
    auto& app = soda::Application::instance();
    auto result = app.initialize(options);
    if (!result.ok()) {
        std::cerr << "Failed to initialize SODA Player: " << result.error() << "\n";
        return 1;
    }

    // Queue files to play if any were specified
    if (!filesToPlay.empty()) {
        auto& engine = app.audioEngine();
        for (const auto& file : filesToPlay) {
            soda::TrackInfo track;
            track.filePath = file;
            track.source = soda::SourceType::Local;
            engine.queue().add(track);
        }
        if (!engine.queue().isEmpty()) {
            engine.play();
        }
    }

    // Run main loop
    app.run();

    // Cleanup
    app.shutdown();

    return 0;
}
