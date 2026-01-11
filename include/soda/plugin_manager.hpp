/*
 * SODA Player - Plugin Manager
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_PLUGIN_MANAGER_HPP
#define SODA_PLUGIN_MANAGER_HPP

#include "soda.hpp"
#include "plugin_interface.hpp"

namespace soda {

class Application;

class PluginManager {
public:
    explicit PluginManager(Application& app);
    ~PluginManager();

    // Initialize plugin system
    Result<void> initialize(const Path& pluginsDir);

    // Shutdown all plugins
    void shutdown();

    // Plugin discovery
    std::vector<PluginInfo> discover();
    std::vector<PluginInfo> installed() const;

    // Plugin lifecycle
    Result<void> load(const std::string& pluginId);
    Result<void> unload(const std::string& pluginId);
    Result<void> enable(const std::string& pluginId);
    Result<void> disable(const std::string& pluginId);

    // Check if plugin is loaded
    bool isLoaded(const std::string& pluginId) const;
    bool isEnabled(const std::string& pluginId) const;

    // Get plugin info
    std::optional<PluginInfo> info(const std::string& pluginId) const;

    // Get plugin instance (for interacting with plugin API)
    PluginInterface* get(const std::string& pluginId);

    // Plugin communication
    void broadcastEvent(const Event& event);

    // Install/Uninstall from archive
    Result<void> install(const Path& archivePath);
    Result<void> uninstall(const std::string& pluginId);

    // Plugin permissions
    std::vector<std::string> requiredPermissions(const std::string& pluginId) const;
    bool hasPermission(const std::string& pluginId, const std::string& permission) const;
    void grantPermission(const std::string& pluginId, const std::string& permission);
    void revokePermission(const std::string& pluginId, const std::string& permission);

private:
    struct LoadedPlugin {
        PluginInfo info;
        void* handle = nullptr; // dlopen handle
        std::unique_ptr<PluginInterface> instance;
        std::vector<std::string> grantedPermissions;
    };

    Result<void> loadPluginManifest(const Path& pluginDir, PluginInfo& info);
    Result<void> validatePlugin(const PluginInfo& info);
    Result<void> loadPluginLibrary(LoadedPlugin& plugin);

    Application& app_;
    Path pluginsDir_;
    std::unordered_map<std::string, LoadedPlugin> plugins_;
    mutable std::mutex mutex_;
};

} // namespace soda

#endif // SODA_PLUGIN_MANAGER_HPP
