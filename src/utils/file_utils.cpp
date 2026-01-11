/*
 * SODA Player - File Utilities Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/file_utils.hpp>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <random>
#include <sys/inotify.h>
#include <unistd.h>

namespace soda {
namespace file_utils {

Result<void> createDirectories(const Path& path) {
    try {
        std::filesystem::create_directories(path);
        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>(std::string("Failed to create directories: ") + e.what());
    }
}

bool exists(const Path& path) {
    return std::filesystem::exists(path);
}

bool isDirectory(const Path& path) {
    return std::filesystem::is_directory(path);
}

bool isFile(const Path& path) {
    return std::filesystem::is_regular_file(path);
}

Result<void> remove(const Path& path) {
    try {
        std::filesystem::remove(path);
        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>(std::string("Failed to remove: ") + e.what());
    }
}

Result<void> removeAll(const Path& path) {
    try {
        std::filesystem::remove_all(path);
        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>(std::string("Failed to remove: ") + e.what());
    }
}

Result<void> copy(const Path& source, const Path& destination) {
    try {
        std::filesystem::copy(source, destination, std::filesystem::copy_options::recursive);
        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>(std::string("Failed to copy: ") + e.what());
    }
}

Result<void> move(const Path& source, const Path& destination) {
    try {
        std::filesystem::rename(source, destination);
        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>(std::string("Failed to move: ") + e.what());
    }
}

Result<void> rename(const Path& source, const Path& destination) {
    return move(source, destination);
}

Result<std::string> readTextFile(const Path& path) {
    std::ifstream file(path);
    if (!file) {
        return Result<std::string>("Failed to open file: " + path.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return Result<std::string>(buffer.str());
}

Result<std::vector<uint8_t>> readBinaryFile(const Path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return Result<std::vector<uint8_t>>("Failed to open file: " + path.string());
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return Result<std::vector<uint8_t>>("Failed to read file");
    }

    return Result<std::vector<uint8_t>>(buffer);
}

Result<void> writeTextFile(const Path& path, const std::string& content) {
    std::ofstream file(path);
    if (!file) {
        return Result<void>("Failed to open file for writing: " + path.string());
    }

    file << content;
    return Result<void>();
}

Result<void> writeBinaryFile(const Path& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return Result<void>("Failed to open file for writing: " + path.string());
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return Result<void>();
}

Result<void> appendTextFile(const Path& path, const std::string& content) {
    std::ofstream file(path, std::ios::app);
    if (!file) {
        return Result<void>("Failed to open file for appending: " + path.string());
    }

    file << content;
    return Result<void>();
}

size_t fileSize(const Path& path) {
    try {
        return std::filesystem::file_size(path);
    } catch (...) {
        return 0;
    }
}

TimePoint lastModified(const Path& path) {
    try {
        auto ftime = std::filesystem::last_write_time(path);
        auto sctp = std::chrono::time_point_cast<std::chrono::steady_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::steady_clock::now()
        );
        return sctp;
    } catch (...) {
        return TimePoint();
    }
}

std::string fileName(const Path& path) {
    return path.filename().string();
}

std::string extension(const Path& path) {
    return path.extension().string();
}

Path parent(const Path& path) {
    return path.parent_path();
}

Path stem(const Path& path) {
    return path.stem();
}

Path normalize(const Path& path) {
    return std::filesystem::weakly_canonical(path);
}

Path absolute(const Path& path) {
    return std::filesystem::absolute(path);
}

Path relative(const Path& path, const Path& base) {
    return std::filesystem::relative(path, base);
}

Path join(const Path& base, const std::string& component) {
    return base / component;
}

bool isAbsolute(const Path& path) {
    return path.is_absolute();
}

std::vector<Path> listFiles(const Path& directory, bool recursive) {
    std::vector<Path> result;

    if (!std::filesystem::exists(directory)) {
        return result;
    }

    try {
        if (recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    result.push_back(entry.path());
                }
            }
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    result.push_back(entry.path());
                }
            }
        }
    } catch (...) {}

    return result;
}

std::vector<Path> listFilesWithExtension(const Path& directory,
                                          const std::string& extension,
                                          bool recursive) {
    std::vector<Path> result;
    auto files = listFiles(directory, recursive);

    for (const auto& file : files) {
        if (file.extension() == extension) {
            result.push_back(file);
        }
    }

    return result;
}

std::vector<Path> listFilesWithExtensions(const Path& directory,
                                           const std::vector<std::string>& extensions,
                                           bool recursive) {
    std::vector<Path> result;
    auto files = listFiles(directory, recursive);

    for (const auto& file : files) {
        std::string ext = file.extension().string();
        if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
            result.push_back(file);
        }
    }

    return result;
}

std::vector<Path> listDirectories(const Path& directory) {
    std::vector<Path> result;

    if (!std::filesystem::exists(directory)) {
        return result;
    }

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_directory()) {
                result.push_back(entry.path());
            }
        }
    } catch (...) {}

    return result;
}

Path tempDirectory() {
    return std::filesystem::temp_directory_path();
}

Path createTempFile(const std::string& prefix, const std::string& suffix) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 999999);

    Path tempDir = tempDirectory();
    Path tempFile;

    do {
        tempFile = tempDir / (prefix + std::to_string(dist(gen)) + suffix);
    } while (std::filesystem::exists(tempFile));

    std::ofstream file(tempFile);
    return tempFile;
}

Path createTempDirectory(const std::string& prefix) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 999999);

    Path tempDir = tempDirectory();
    Path newDir;

    do {
        newDir = tempDir / (prefix + std::to_string(dist(gen)));
    } while (std::filesystem::exists(newDir));

    std::filesystem::create_directories(newDir);
    return newDir;
}

Path getConfigDir() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg) {
        return Path(xdg);
    }

    const char* home = std::getenv("HOME");
    if (home) {
        return Path(home) / ".config";
    }

    return Path(".");
}

Path getDataDir() {
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg) {
        return Path(xdg);
    }

    const char* home = std::getenv("HOME");
    if (home) {
        return Path(home) / ".local" / "share";
    }

    return Path(".");
}

Path getCacheDir() {
    const char* xdg = std::getenv("XDG_CACHE_HOME");
    if (xdg) {
        return Path(xdg);
    }

    const char* home = std::getenv("HOME");
    if (home) {
        return Path(home) / ".cache";
    }

    return Path(".");
}

Path getMusicDir() {
    const char* home = std::getenv("HOME");
    if (home) {
        return Path(home) / "Music";
    }
    return Path(".");
}

std::string sanitizeFileName(const std::string& name) {
    std::string result;
    result.reserve(name.size());

    for (char c : name) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' ||
            c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            result += '_';
        } else {
            result += c;
        }
    }

    return result;
}

Path uniqueFileName(const Path& path) {
    if (!std::filesystem::exists(path)) {
        return path;
    }

    Path parent = path.parent_path();
    std::string stem = path.stem().string();
    std::string ext = path.extension().string();

    int counter = 1;
    Path newPath;

    do {
        newPath = parent / (stem + " (" + std::to_string(counter) + ")" + ext);
        ++counter;
    } while (std::filesystem::exists(newPath));

    return newPath;
}

std::string md5(const Path& path) {
    // Simplified implementation - in production use a proper MD5 library
    auto content = readBinaryFile(path);
    if (!content.ok()) {
        return "";
    }

    // This is a placeholder - use actual MD5 implementation
    size_t hash = std::hash<std::string>{}(std::string(content.value().begin(), content.value().end()));
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

std::string sha256(const Path& path) {
    // Placeholder - use actual SHA256 implementation
    return md5(path);
}

int watchDirectory(const Path& directory, std::function<void(const Path&)> callback) {
    int fd = inotify_init();
    if (fd < 0) {
        return -1;
    }

    int wd = inotify_add_watch(fd, directory.c_str(),
                               IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO);
    if (wd < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

void stopWatching(int watchFd) {
    if (watchFd >= 0) {
        close(watchFd);
    }
}

} // namespace file_utils
} // namespace soda
