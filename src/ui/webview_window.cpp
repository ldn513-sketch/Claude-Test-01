/*
 * SODA Player - WebView Window Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/webview_window.hpp>
#include <soda/js_bridge.hpp>
#include <soda/skin_manager.hpp>
#include <soda/event_bus.hpp>
#include <soda/application.hpp>

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

namespace soda {

// Static callbacks for GTK/WebKit
static gboolean on_close_request(GtkWidget* widget, gpointer user_data) {
    WebViewWindow* window = static_cast<WebViewWindow*>(user_data);
    window->close();
    return FALSE;
}

static void on_script_message_received(WebKitUserContentManager* manager,
                                        WebKitJavascriptResult* js_result,
                                        gpointer user_data) {
    WebViewWindow* window = static_cast<WebViewWindow*>(user_data);

    JSCValue* value = webkit_javascript_result_get_js_value(js_result);
    if (jsc_value_is_string(value)) {
        gchar* message = jsc_value_to_string(value);
        if (message) {
            JsResponse response = window->jsBridge().handleMessage(message);

            // Send response back to JavaScript
            std::string responseJs = "window.soda._handleResponse('" +
                                     response.callbackId + "');";
            // Would execute responseJs here

            g_free(message);
        }
    }
}

WebViewWindow::WebViewWindow(EventBus& eventBus, SkinManager& skinManager)
    : eventBus_(eventBus)
    , skinManager_(skinManager)
    , jsBridge_(std::make_unique<JsBridge>(Application::instance()))
{
}

WebViewWindow::~WebViewWindow() {
    if (gtkWindow_) {
        // Cleanup handled by GTK
    }
}

Result<void> WebViewWindow::initialize(const WindowConfig& config) {
    config_ = config;

    // Initialize GTK if not already done
    if (!gtk_init_check()) {
        return Result<void>("Failed to initialize GTK");
    }

    setupGtkWindow();
    setupWebView();
    connectSignals();

    loadSkin();

    initialized_ = true;
    return Result<void>();
}

void WebViewWindow::setupGtkWindow() {
    GtkWidget* window = gtk_window_new();
    gtkWindow_ = window;

    gtk_window_set_title(GTK_WINDOW(window), config_.title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(window), config_.width, config_.height);

    if (config_.minWidth > 0 && config_.minHeight > 0) {
        gtk_widget_set_size_request(window, config_.minWidth, config_.minHeight);
    }

    gtk_window_set_resizable(GTK_WINDOW(window), config_.resizable);
    gtk_window_set_decorated(GTK_WINDOW(window), config_.decorated);

    if (config_.x.has_value() && config_.y.has_value()) {
        // GTK4 doesn't have direct position setting, would need GDK
    }
}

void WebViewWindow::setupWebView() {
    // Create WebKit settings
    WebKitSettings* settings = webkit_settings_new();
    webkit_settings_set_javascript_can_access_clipboard(settings, TRUE);
    webkit_settings_set_enable_developer_extras(settings, TRUE);

    // Create user content manager for JS bridge
    WebKitUserContentManager* contentManager = webkit_user_content_manager_new();

    // Register message handler
    webkit_user_content_manager_register_script_message_handler(contentManager, "soda", NULL);

    g_signal_connect(contentManager, "script-message-received::soda",
                     G_CALLBACK(on_script_message_received), this);

    // Create WebView
    GtkWidget* webView = webkit_web_view_new_with_user_content_manager(contentManager);
    webView_ = webView;

    webkit_web_view_set_settings(WEBKIT_WEB_VIEW(webView), settings);

    // Add WebView to window
    gtk_window_set_child(GTK_WINDOW(gtkWindow_), webView);

    g_object_unref(settings);
}

void WebViewWindow::connectSignals() {
    g_signal_connect(gtkWindow_, "close-request", G_CALLBACK(on_close_request), this);

    // Subscribe to events to push to JS
    eventBus_.subscribeAll([this](const Event& event) {
        std::string eventJs = jsBridge_->createEventJs(event);
        executeJs(eventJs);
    });
}

void WebViewWindow::injectJsBridge() {
    std::string apiCode = jsBridge_->generateApiCode();
    executeJs(apiCode);
}

void WebViewWindow::run() {
    if (!initialized_) {
        return;
    }

    running_ = true;

    gtk_widget_set_visible(static_cast<GtkWidget*>(gtkWindow_), TRUE);

    // GTK4 main loop
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
}

void WebViewWindow::close() {
    running_ = false;

    if (gtkWindow_) {
        gtk_window_close(GTK_WINDOW(gtkWindow_));
    }

    // Quit GTK main loop
    GMainContext* context = g_main_context_default();
    while (g_main_context_iteration(context, FALSE)) {}
}

bool WebViewWindow::isVisible() const {
    if (!gtkWindow_) return false;
    return gtk_widget_get_visible(static_cast<GtkWidget*>(gtkWindow_));
}

void WebViewWindow::show() {
    if (gtkWindow_) {
        gtk_widget_set_visible(static_cast<GtkWidget*>(gtkWindow_), TRUE);
    }
}

void WebViewWindow::hide() {
    if (gtkWindow_) {
        gtk_widget_set_visible(static_cast<GtkWidget*>(gtkWindow_), FALSE);
    }
}

void WebViewWindow::minimize() {
    if (gtkWindow_) {
        gtk_window_minimize(GTK_WINDOW(gtkWindow_));
    }
}

void WebViewWindow::maximize() {
    if (gtkWindow_) {
        gtk_window_maximize(GTK_WINDOW(gtkWindow_));
    }
}

void WebViewWindow::restore() {
    if (gtkWindow_) {
        gtk_window_unmaximize(GTK_WINDOW(gtkWindow_));
    }
}

void WebViewWindow::setFullscreen(bool fullscreen) {
    if (gtkWindow_) {
        gtk_window_set_fullscreened(GTK_WINDOW(gtkWindow_), fullscreen);
    }
}

bool WebViewWindow::isFullscreen() const {
    if (!gtkWindow_) return false;
    return gtk_window_is_fullscreen(GTK_WINDOW(gtkWindow_));
}

void WebViewWindow::setTitle(const std::string& title) {
    if (gtkWindow_) {
        gtk_window_set_title(GTK_WINDOW(gtkWindow_), title.c_str());
    }
}

void WebViewWindow::setSize(int width, int height) {
    if (gtkWindow_) {
        gtk_window_set_default_size(GTK_WINDOW(gtkWindow_), width, height);
    }
}

void WebViewWindow::setPosition(int x, int y) {
    // GTK4 doesn't support direct window positioning
}

std::pair<int, int> WebViewWindow::getSize() const {
    if (!gtkWindow_) return {0, 0};
    int width, height;
    gtk_window_get_default_size(GTK_WINDOW(gtkWindow_), &width, &height);
    return {width, height};
}

std::pair<int, int> WebViewWindow::getPosition() const {
    return {0, 0}; // GTK4 doesn't expose window position
}

void WebViewWindow::loadSkin() {
    std::string html = skinManager_.getFullPage();
    loadHtml(html);
}

void WebViewWindow::loadHtml(const std::string& html) {
    if (webView_) {
        webkit_web_view_load_html(WEBKIT_WEB_VIEW(webView_), html.c_str(), "file:///");
    }
}

void WebViewWindow::loadUrl(const std::string& url) {
    if (webView_) {
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webView_), url.c_str());
    }
}

void WebViewWindow::executeJs(const std::string& js) {
    if (webView_) {
        webkit_web_view_evaluate_javascript(
            WEBKIT_WEB_VIEW(webView_),
            js.c_str(), -1, NULL, NULL, NULL, NULL, NULL);
    }
}

void WebViewWindow::executeJs(const std::string& js, std::function<void(const std::string&)> callback) {
    // For now, just execute without callback handling
    executeJs(js);
}

void WebViewWindow::showTrayIcon(bool show) {
    // System tray implementation would go here
}

void WebViewWindow::setTrayTooltip(const std::string& tooltip) {
    // System tray implementation
}

void WebViewWindow::showNotification(const std::string& title, const std::string& body) {
    // Desktop notification using libnotify or similar
    // For now, just log it
}

} // namespace soda
