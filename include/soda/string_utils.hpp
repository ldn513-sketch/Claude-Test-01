/*
 * SODA Player - String Utilities
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_STRING_UTILS_HPP
#define SODA_STRING_UTILS_HPP

#include "soda.hpp"

namespace soda {
namespace string_utils {

// Trimming
std::string trim(const std::string& str);
std::string trimLeft(const std::string& str);
std::string trimRight(const std::string& str);

// Case conversion
std::string toLower(const std::string& str);
std::string toUpper(const std::string& str);
std::string toTitleCase(const std::string& str);

// Comparison
bool equalsIgnoreCase(const std::string& a, const std::string& b);
bool startsWith(const std::string& str, const std::string& prefix);
bool endsWith(const std::string& str, const std::string& suffix);
bool contains(const std::string& str, const std::string& substr);
bool containsIgnoreCase(const std::string& str, const std::string& substr);

// Splitting and joining
std::vector<std::string> split(const std::string& str, char delimiter);
std::vector<std::string> split(const std::string& str, const std::string& delimiter);
std::string join(const std::vector<std::string>& parts, const std::string& delimiter);

// Replacement
std::string replace(const std::string& str, const std::string& from, const std::string& to);
std::string replaceAll(const std::string& str, const std::string& from, const std::string& to);

// Formatting
std::string formatDuration(Duration duration);        // "3:45" or "1:23:45"
std::string formatDurationLong(Duration duration);    // "3 minutes 45 seconds"
std::string formatSize(size_t bytes);                 // "1.5 MB"
std::string formatDate(TimePoint time);               // "2026-01-11"
std::string formatDateTime(TimePoint time);           // "2026-01-11 14:30:00"
std::string formatRelativeTime(TimePoint time);       // "2 hours ago"

// Parsing
Duration parseDuration(const std::string& str);       // "3:45" or "1h30m"
size_t parseSize(const std::string& str);             // "1.5MB", "500KB"
std::optional<int> parseInt(const std::string& str);
std::optional<double> parseDouble(const std::string& str);

// Encoding
std::string base64Encode(const std::vector<uint8_t>& data);
std::string base64Encode(const std::string& str);
std::vector<uint8_t> base64Decode(const std::string& encoded);
std::string base64DecodeString(const std::string& encoded);

// HTML/XML
std::string escapeHtml(const std::string& str);
std::string unescapeHtml(const std::string& str);
std::string escapeXml(const std::string& str);
std::string stripHtmlTags(const std::string& html);

// JSON (simple operations)
std::string escapeJson(const std::string& str);

// UUID generation
std::string generateUuid();

// Hashing
std::string md5(const std::string& str);
std::string sha256(const std::string& str);

// Levenshtein distance (for fuzzy matching)
int levenshteinDistance(const std::string& a, const std::string& b);

// Slug generation (for safe filenames/URLs)
std::string slugify(const std::string& str);

// Word wrap for display
std::vector<std::string> wordWrap(const std::string& text, size_t maxWidth);

} // namespace string_utils
} // namespace soda

#endif // SODA_STRING_UTILS_HPP
