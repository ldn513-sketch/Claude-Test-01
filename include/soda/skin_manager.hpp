/*
 * SODA Player - Skin Manager
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_SKIN_MANAGER_HPP
#define SODA_SKIN_MANAGER_HPP

#include "soda.hpp"

namespace soda {

class EventBus;
class ConfigManager;

// Skin metadata
struct SkinInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    Path path;
    std::optional<std::string> screenshot; // Base64 or path to screenshot
    std::vector<std::string> tags; // "dark", "light", "minimal", etc.
};

class SkinManager {
public:
    SkinManager(EventBus& eventBus, ConfigManager& config);
    ~SkinManager();

    // Initialize with skins directory
    Result<void> initialize(const Path& skinsDir);

    // Skin discovery
    std::vector<SkinInfo> availableSkins() const;
    std::optional<SkinInfo> getSkinInfo(const std::string& skinId) const;

    // Current skin
    const std::string& currentSkinId() const { return currentSkinId_; }
    Result<void> setSkin(const std::string& skinId);
    void reloadCurrentSkin();

    // Get skin files
    std::string getHtml() const;
    std::string getCss() const;
    std::string getJs() const;

    // Get full HTML page (combines HTML, CSS, JS with API bindings)
    std::string getFullPage() const;

    // Install skin from archive
    Result<void> installSkin(const Path& archivePath);
    Result<void> uninstallSkin(const std::string& skinId);

    // Create new skin from template
    Result<void> createSkin(const std::string& name, const std::string& baseTheme = "default-dark");

    // Skin validation
    bool validateSkin(const Path& skinDir) const;

private:
    Result<SkinInfo> loadSkinInfo(const Path& skinDir);
    void scanSkins();
    std::string generateApiBindings() const;
    std::string wrapInHtmlDocument(const std::string& bodyContent,
                                    const std::string& css,
                                    const std::string& js) const;

    EventBus& eventBus_;
    ConfigManager& config_;
    Path skinsDir_;

    std::string currentSkinId_;
    std::unordered_map<std::string, SkinInfo> skins_;

    // Cached content of current skin
    std::string cachedHtml_;
    std::string cachedCss_;
    std::string cachedJs_;

    mutable std::mutex mutex_;
};

} // namespace soda

#endif // SODA_SKIN_MANAGER_HPP
