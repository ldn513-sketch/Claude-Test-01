/*
 * SODA Player - WebView Window
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_WEBVIEW_WINDOW_HPP
#define SODA_WEBVIEW_WINDOW_HPP

#include "soda.hpp"

namespace soda {

class JsBridge;
class SkinManager;
class EventBus;

// Window configuration
struct WindowConfig {
    std::string title = "SODA Player";
    int width = 1200;
    int height = 800;
    int minWidth = 400;
    int minHeight = 300;
    bool resizable = true;
    bool decorated = true;
    bool transparent = false;
    std::optional<int> x;
    std::optional<int> y;
};

class WebViewWindow {
public:
    WebViewWindow(EventBus& eventBus, SkinManager& skinManager);
    ~WebViewWindow();

    // Initialize window
    Result<void> initialize(const WindowConfig& config = {});

    // Run the main loop (blocking)
    void run();

    // Close window and exit main loop
    void close();

    // Window state
    bool isVisible() const;
    void show();
    void hide();
    void minimize();
    void maximize();
    void restore();
    void setFullscreen(bool fullscreen);
    bool isFullscreen() const;

    // Window properties
    void setTitle(const std::string& title);
    void setSize(int width, int height);
    void setPosition(int x, int y);
    std::pair<int, int> getSize() const;
    std::pair<int, int> getPosition() const;

    // Load content
    void loadSkin();
    void loadHtml(const std::string& html);
    void loadUrl(const std::string& url);

    // JavaScript interaction
    void executeJs(const std::string& js);
    void executeJs(const std::string& js, std::function<void(const std::string&)> callback);

    // JS Bridge access
    JsBridge& jsBridge() { return *jsBridge_; }

    // System tray (optional)
    void showTrayIcon(bool show);
    void setTrayTooltip(const std::string& tooltip);

    // Notifications
    void showNotification(const std::string& title, const std::string& body);

private:
    void setupGtkWindow();
    void setupWebView();
    void connectSignals();
    void injectJsBridge();

    EventBus& eventBus_;
    SkinManager& skinManager_;
    std::unique_ptr<JsBridge> jsBridge_;

    // GTK/WebKit handles (void* to avoid header dependencies)
    void* gtkWindow_ = nullptr;
    void* webView_ = nullptr;
    void* webContext_ = nullptr;

    WindowConfig config_;
    bool initialized_ = false;
    bool running_ = false;

    mutable std::mutex mutex_;
};

} // namespace soda

#endif // SODA_WEBVIEW_WINDOW_HPP
