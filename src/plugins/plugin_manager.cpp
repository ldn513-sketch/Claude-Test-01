/*
 * SODA Player - Plugin Manager Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/plugin_manager.hpp>
#include <soda/application.hpp>
#include <yaml-cpp/yaml.h>
#include <dlfcn.h>
#include <fstream>

namespace soda {

PluginManager::PluginManager(Application& app)
    : app_(app)
{
}

PluginManager::~PluginManager() {
    shutdown();
}

Result<void> PluginManager::initialize(const Path& pluginsDir) {
    pluginsDir_ = pluginsDir;
    std::filesystem::create_directories(pluginsDir_);

    // Discover and auto-load enabled plugins
    auto discovered = discover();
    for (const auto& info : discovered) {
        if (info.isEnabled) {
            auto result = load(info.id);
            if (!result.ok()) {
                // Log error but continue with other plugins
                app_.events().emitError("Failed to load plugin " + info.name + ": " + result.error());
            }
        }
    }

    return Result<void>();
}

void PluginManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [id, plugin] : plugins_) {
        if (plugin.instance) {
            plugin.instance->shutdown();
        }
        if (plugin.handle) {
            dlclose(plugin.handle);
        }
    }

    plugins_.clear();
}

std::vector<PluginInfo> PluginManager::discover() {
    std::vector<PluginInfo> result;
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        for (const auto& entry : std::filesystem::directory_iterator(pluginsDir_)) {
            if (!entry.is_directory()) continue;

            PluginInfo info;
            auto loadResult = loadPluginManifest(entry.path(), info);
            if (loadResult.ok()) {
                result.push_back(info);
            }
        }
    } catch (...) {
        // Ignore filesystem errors
    }

    return result;
}

std::vector<PluginInfo> PluginManager::installed() const {
    std::vector<PluginInfo> result;
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& [id, plugin] : plugins_) {
        result.push_back(plugin.info);
    }

    return result;
}

Result<void> PluginManager::loadPluginManifest(const Path& pluginDir, PluginInfo& info) {
    Path manifestPath = pluginDir / "manifest.yaml";
    if (!std::filesystem::exists(manifestPath)) {
        return Result<void>("No manifest.yaml found");
    }

    try {
        YAML::Node manifest = YAML::LoadFile(manifestPath.string());

        info.id = manifest["id"].as<std::string>();
        info.name = manifest["name"].as<std::string>();
        info.version = manifest["version"].as<std::string>("1.0.0");
        info.author = manifest["author"].as<std::string>("");
        info.description = manifest["description"].as<std::string>("");
        info.path = pluginDir;
        info.isEnabled = manifest["enabled"].as<bool>(false);

        if (manifest["permissions"]) {
            for (const auto& perm : manifest["permissions"]) {
                info.permissions.push_back(perm.as<std::string>());
            }
        }

        return Result<void>();

    } catch (const std::exception& e) {
        return Result<void>("Failed to parse manifest: " + std::string(e.what()));
    }
}

Result<void> PluginManager::validatePlugin(const PluginInfo& info) {
    if (info.id.empty()) {
        return Result<void>("Plugin ID is required");
    }
    if (info.name.empty()) {
        return Result<void>("Plugin name is required");
    }

    // Check for library file
    Path libPath = info.path / ("lib" + info.id + ".so");
    if (!std::filesystem::exists(libPath)) {
        return Result<void>("Plugin library not found: " + libPath.string());
    }

    return Result<void>();
}

Result<void> PluginManager::loadPluginLibrary(LoadedPlugin& plugin) {
    Path libPath = plugin.info.path / ("lib" + plugin.info.id + ".so");

    plugin.handle = dlopen(libPath.c_str(), RTLD_LAZY);
    if (!plugin.handle) {
        return Result<void>("Failed to load library: " + std::string(dlerror()));
    }

    // Check API version
    auto apiVersionFn = reinterpret_cast<int(*)()>(dlsym(plugin.handle, "soda_plugin_api_version"));
    if (apiVersionFn && apiVersionFn() != SODA_PLUGIN_API_VERSION) {
        dlclose(plugin.handle);
        plugin.handle = nullptr;
        return Result<void>("Plugin API version mismatch");
    }

    // Create plugin instance
    auto createFn = reinterpret_cast<PluginInterface*(*)()>(dlsym(plugin.handle, "soda_plugin_create"));
    if (!createFn) {
        dlclose(plugin.handle);
        plugin.handle = nullptr;
        return Result<void>("Plugin does not export soda_plugin_create");
    }

    plugin.instance.reset(createFn());
    if (!plugin.instance) {
        dlclose(plugin.handle);
        plugin.handle = nullptr;
        return Result<void>("Failed to create plugin instance");
    }

    return Result<void>();
}

Result<void> PluginManager::load(const std::string& pluginId) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if already loaded
    if (plugins_.find(pluginId) != plugins_.end()) {
        return Result<void>();
    }

    // Find plugin directory
    Path pluginDir = pluginsDir_ / pluginId;
    if (!std::filesystem::exists(pluginDir)) {
        return Result<void>("Plugin not found: " + pluginId);
    }

    LoadedPlugin plugin;
    auto manifestResult = loadPluginManifest(pluginDir, plugin.info);
    if (!manifestResult.ok()) {
        return manifestResult;
    }

    auto validateResult = validatePlugin(plugin.info);
    if (!validateResult.ok()) {
        return validateResult;
    }

    auto loadResult = loadPluginLibrary(plugin);
    if (!loadResult.ok()) {
        return loadResult;
    }

    // Initialize plugin
    auto initResult = plugin.instance->initialize(app_);
    if (!initResult.ok()) {
        if (plugin.handle) {
            dlclose(plugin.handle);
        }
        return Result<void>("Plugin initialization failed: " + initResult.error());
    }

    plugins_[pluginId] = std::move(plugin);

    // Emit event
    Event event;
    event.type = EventType::PluginLoaded;
    event.data = pluginId;
    app_.events().publish(event);

    return Result<void>();
}

Result<void> PluginManager::unload(const std::string& pluginId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        return Result<void>("Plugin not loaded");
    }

    if (it->second.instance) {
        it->second.instance->shutdown();
    }
    if (it->second.handle) {
        dlclose(it->second.handle);
    }

    plugins_.erase(it);

    Event event;
    event.type = EventType::PluginUnloaded;
    event.data = pluginId;
    app_.events().publish(event);

    return Result<void>();
}

Result<void> PluginManager::enable(const std::string& pluginId) {
    auto it = plugins_.find(pluginId);
    if (it != plugins_.end() && it->second.instance) {
        it->second.instance->onEnable();
        it->second.info.isEnabled = true;
    }
    return Result<void>();
}

Result<void> PluginManager::disable(const std::string& pluginId) {
    auto it = plugins_.find(pluginId);
    if (it != plugins_.end() && it->second.instance) {
        it->second.instance->onDisable();
        it->second.info.isEnabled = false;
    }
    return Result<void>();
}

bool PluginManager::isLoaded(const std::string& pluginId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return plugins_.find(pluginId) != plugins_.end();
}

bool PluginManager::isEnabled(const std::string& pluginId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(pluginId);
    return it != plugins_.end() && it->second.info.isEnabled;
}

std::optional<PluginInfo> PluginManager::info(const std::string& pluginId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(pluginId);
    if (it != plugins_.end()) {
        return it->second.info;
    }
    return std::nullopt;
}

PluginInterface* PluginManager::get(const std::string& pluginId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(pluginId);
    if (it != plugins_.end()) {
        return it->second.instance.get();
    }
    return nullptr;
}

void PluginManager::broadcastEvent(const Event& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, plugin] : plugins_) {
        if (plugin.instance && plugin.info.isEnabled) {
            try {
                plugin.instance->onEvent(event);
            } catch (...) {
                // Ignore plugin exceptions
            }
        }
    }
}

Result<void> PluginManager::install(const Path& archivePath) {
    // TODO: Implement archive extraction
    return Result<void>("Archive installation not yet implemented");
}

Result<void> PluginManager::uninstall(const std::string& pluginId) {
    auto unloadResult = unload(pluginId);

    Path pluginDir = pluginsDir_ / pluginId;
    if (std::filesystem::exists(pluginDir)) {
        std::filesystem::remove_all(pluginDir);
    }

    return Result<void>();
}

std::vector<std::string> PluginManager::requiredPermissions(const std::string& pluginId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(pluginId);
    if (it != plugins_.end()) {
        return it->second.info.permissions;
    }
    return {};
}

bool PluginManager::hasPermission(const std::string& pluginId, const std::string& permission) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(pluginId);
    if (it != plugins_.end()) {
        const auto& perms = it->second.grantedPermissions;
        return std::find(perms.begin(), perms.end(), permission) != perms.end();
    }
    return false;
}

void PluginManager::grantPermission(const std::string& pluginId, const std::string& permission) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(pluginId);
    if (it != plugins_.end()) {
        auto& perms = it->second.grantedPermissions;
        if (std::find(perms.begin(), perms.end(), permission) == perms.end()) {
            perms.push_back(permission);
        }
    }
}

void PluginManager::revokePermission(const std::string& pluginId, const std::string& permission) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(pluginId);
    if (it != plugins_.end()) {
        auto& perms = it->second.grantedPermissions;
        perms.erase(std::remove(perms.begin(), perms.end(), permission), perms.end());
    }
}

} // namespace soda
