/*
 * SODA Player - File Utilities
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_FILE_UTILS_HPP
#define SODA_FILE_UTILS_HPP

#include "soda.hpp"

namespace soda {
namespace file_utils {

// Directory operations
Result<void> createDirectories(const Path& path);
bool exists(const Path& path);
bool isDirectory(const Path& path);
bool isFile(const Path& path);
Result<void> remove(const Path& path);
Result<void> removeAll(const Path& path);
Result<void> copy(const Path& source, const Path& destination);
Result<void> move(const Path& source, const Path& destination);
Result<void> rename(const Path& source, const Path& destination);

// File operations
Result<std::string> readTextFile(const Path& path);
Result<std::vector<uint8_t>> readBinaryFile(const Path& path);
Result<void> writeTextFile(const Path& path, const std::string& content);
Result<void> writeBinaryFile(const Path& path, const std::vector<uint8_t>& data);
Result<void> appendTextFile(const Path& path, const std::string& content);

// File info
size_t fileSize(const Path& path);
TimePoint lastModified(const Path& path);
std::string fileName(const Path& path);
std::string extension(const Path& path);
Path parent(const Path& path);
Path stem(const Path& path);

// Path manipulation
Path normalize(const Path& path);
Path absolute(const Path& path);
Path relative(const Path& path, const Path& base);
Path join(const Path& base, const std::string& component);
bool isAbsolute(const Path& path);

// Directory listing
std::vector<Path> listFiles(const Path& directory, bool recursive = false);
std::vector<Path> listFilesWithExtension(const Path& directory,
                                          const std::string& extension,
                                          bool recursive = false);
std::vector<Path> listFilesWithExtensions(const Path& directory,
                                           const std::vector<std::string>& extensions,
                                           bool recursive = false);
std::vector<Path> listDirectories(const Path& directory);

// Glob pattern matching
std::vector<Path> glob(const Path& directory, const std::string& pattern);

// Temporary files
Path tempDirectory();
Path createTempFile(const std::string& prefix = "soda_", const std::string& suffix = "");
Path createTempDirectory(const std::string& prefix = "soda_");

// XDG directories (Linux)
Path getConfigDir();
Path getDataDir();
Path getCacheDir();
Path getMusicDir();

// Sanitize filename (remove invalid characters)
std::string sanitizeFileName(const std::string& name);

// Generate unique filename if file exists
Path uniqueFileName(const Path& path);

// Calculate file hash
std::string md5(const Path& path);
std::string sha256(const Path& path);

// Watch directory for changes (returns file descriptor for polling)
int watchDirectory(const Path& directory, std::function<void(const Path&)> callback);
void stopWatching(int watchFd);

} // namespace file_utils
} // namespace soda

#endif // SODA_FILE_UTILS_HPP
