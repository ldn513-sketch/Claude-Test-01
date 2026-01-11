cmake_minimum_required(VERSION 3.16)
project(soda-player VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(SODA_BUILD_TESTS "Build tests" ON)
option(SODA_ENABLE_YOUTUBE "Enable YouTube support" ON)
option(SODA_ENABLE_PODCASTS "Enable Podcast support" ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)
pkg_check_modules(TAGLIB REQUIRED taglib)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.1)
pkg_check_modules(GTK4 REQUIRED gtk4)

# miniaudio is header-only, included in third_party
add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE ${CMAKE_SOURCE_DIR}/third_party/miniaudio)

# Core library
add_library(soda_core STATIC
    src/core/application.cpp
    src/core/event_bus.cpp
    src/audio/audio_engine.cpp
    src/audio/audio_decoder.cpp
    src/audio/playlist.cpp
    src/audio/queue.cpp
    src/config/config_manager.cpp
    src/config/settings.cpp
    src/plugins/plugin_manager.cpp
    src/plugins/plugin_interface.cpp
    src/sources/source_manager.cpp
    src/sources/local_source.cpp
    src/sources/youtube_source.cpp
    src/sources/podcast_source.cpp
    src/ui/skin_manager.cpp
    src/ui/webview_window.cpp
    src/ui/js_bridge.cpp
    src/utils/file_utils.cpp
    src/utils/string_utils.cpp
    src/utils/http_client.cpp
    src/utils/metadata_reader.cpp
)

target_include_directories(soda_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${YAML_CPP_INCLUDE_DIRS}
    ${TAGLIB_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${WEBKIT_INCLUDE_DIRS}
    ${GTK4_INCLUDE_DIRS}
)

target_link_libraries(soda_core PUBLIC
    miniaudio
    ${YAML_CPP_LIBRARIES}
    ${TAGLIB_LIBRARIES}
    ${CURL_LIBRARIES}
    ${WEBKIT_LIBRARIES}
    ${GTK4_LIBRARIES}
    pthread
    dl
    m
)

target_compile_options(soda_core PRIVATE
    ${YAML_CPP_CFLAGS_OTHER}
    ${TAGLIB_CFLAGS_OTHER}
    ${CURL_CFLAGS_OTHER}
    ${WEBKIT_CFLAGS_OTHER}
    ${GTK4_CFLAGS_OTHER}
)

# Main executable
add_executable(soda-player src/main.cpp)
target_link_libraries(soda-player PRIVATE soda_core)

# Install rules
install(TARGETS soda-player DESTINATION bin)
install(DIRECTORY skins/ DESTINATION share/soda-player/skins)
install(DIRECTORY config/ DESTINATION share/soda-player/config)

# Tests
if(SODA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# CPack configuration
set(CPACK_PACKAGE_NAME "soda-player")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SODA Player - Streaming and Offline Digital Audio Player")
set(CPACK_PACKAGE_VENDOR "SODA Project")
set(CPACK_PACKAGE_LICENSE "GPL-3.0")
include(CPack)
