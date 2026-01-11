/*
 * SODA Player - String Utilities Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/string_utils.hpp>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <random>
#include <ctime>
#include <cctype>
#include <regex>

namespace soda {
namespace string_utils {

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

std::string trimLeft(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";
    return str.substr(start);
}

std::string trimRight(const std::string& str) {
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    if (end == std::string::npos) return "";
    return str.substr(0, end + 1);
}

std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::string toTitleCase(const std::string& str) {
    std::string result = str;
    bool newWord = true;

    for (char& c : result) {
        if (std::isspace(c)) {
            newWord = true;
        } else if (newWord) {
            c = std::toupper(c);
            newWord = false;
        } else {
            c = std::tolower(c);
        }
    }

    return result;
}

bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    return toLower(a) == toLower(b);
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() &&
           str.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

bool containsIgnoreCase(const std::string& str, const std::string& substr) {
    return contains(toLower(str), toLower(substr));
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }

    return result;
}

std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(delimiter);

    while (end != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }

    result.push_back(str.substr(start));
    return result;
}

std::string join(const std::vector<std::string>& parts, const std::string& delimiter) {
    std::string result;

    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += delimiter;
        result += parts[i];
    }

    return result;
}

std::string replace(const std::string& str, const std::string& from, const std::string& to) {
    std::string result = str;
    size_t pos = result.find(from);

    if (pos != std::string::npos) {
        result.replace(pos, from.length(), to);
    }

    return result;
}

std::string replaceAll(const std::string& str, const std::string& from, const std::string& to) {
    std::string result = str;
    size_t pos = 0;

    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }

    return result;
}

std::string formatDuration(Duration duration) {
    int64_t totalSeconds = duration.count() / 1000;
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    std::stringstream ss;

    if (hours > 0) {
        ss << hours << ":" << std::setfill('0') << std::setw(2) << minutes
           << ":" << std::setw(2) << seconds;
    } else {
        ss << minutes << ":" << std::setfill('0') << std::setw(2) << seconds;
    }

    return ss.str();
}

std::string formatDurationLong(Duration duration) {
    int64_t totalSeconds = duration.count() / 1000;
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    std::stringstream ss;

    if (hours > 0) {
        ss << hours << " hour" << (hours > 1 ? "s" : "") << " ";
    }
    if (minutes > 0) {
        ss << minutes << " minute" << (minutes > 1 ? "s" : "") << " ";
    }
    if (seconds > 0 || (hours == 0 && minutes == 0)) {
        ss << seconds << " second" << (seconds != 1 ? "s" : "");
    }

    return trim(ss.str());
}

std::string formatSize(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024 && unitIndex < 4) {
        size /= 1024;
        ++unitIndex;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(unitIndex > 0 ? 1 : 0) << size << " " << units[unitIndex];
    return ss.str();
}

std::string formatDate(TimePoint time) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        time - Clock::now() + std::chrono::system_clock::now()
    );
    auto timet = std::chrono::system_clock::to_time_t(sctp);
    auto tm = std::localtime(&timet);

    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%d");
    return ss.str();
}

std::string formatDateTime(TimePoint time) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        time - Clock::now() + std::chrono::system_clock::now()
    );
    auto timet = std::chrono::system_clock::to_time_t(sctp);
    auto tm = std::localtime(&timet);

    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string formatRelativeTime(TimePoint time) {
    auto now = Clock::now();
    auto diff = now - time;

    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(diff).count();
    auto minutes = seconds / 60;
    auto hours = minutes / 60;
    auto days = hours / 24;

    if (days > 0) {
        return std::to_string(days) + " day" + (days > 1 ? "s" : "") + " ago";
    }
    if (hours > 0) {
        return std::to_string(hours) + " hour" + (hours > 1 ? "s" : "") + " ago";
    }
    if (minutes > 0) {
        return std::to_string(minutes) + " minute" + (minutes > 1 ? "s" : "") + " ago";
    }
    return "just now";
}

Duration parseDuration(const std::string& str) {
    // Handle formats like "3:45", "1:23:45", "1h30m", etc.
    std::regex hhmmss(R"((\d+):(\d+):(\d+))");
    std::regex mmss(R"((\d+):(\d+))");
    std::regex hms(R"((?:(\d+)h)?(?:(\d+)m)?(?:(\d+)s)?)");

    std::smatch match;

    if (std::regex_match(str, match, hhmmss)) {
        int hours = std::stoi(match[1].str());
        int minutes = std::stoi(match[2].str());
        int seconds = std::stoi(match[3].str());
        return Duration((hours * 3600 + minutes * 60 + seconds) * 1000);
    }

    if (std::regex_match(str, match, mmss)) {
        int minutes = std::stoi(match[1].str());
        int seconds = std::stoi(match[2].str());
        return Duration((minutes * 60 + seconds) * 1000);
    }

    return Duration(0);
}

size_t parseSize(const std::string& str) {
    std::regex pattern(R"((\d+(?:\.\d+)?)\s*(B|KB|MB|GB|TB)?)", std::regex::icase);
    std::smatch match;

    if (std::regex_match(str, match, pattern)) {
        double value = std::stod(match[1].str());
        std::string unit = toUpper(match[2].str());

        if (unit == "KB") value *= 1024;
        else if (unit == "MB") value *= 1024 * 1024;
        else if (unit == "GB") value *= 1024 * 1024 * 1024;
        else if (unit == "TB") value *= 1024ULL * 1024 * 1024 * 1024;

        return static_cast<size_t>(value);
    }

    return 0;
}

std::optional<int> parseInt(const std::string& str) {
    try {
        return std::stoi(str);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<double> parseDouble(const std::string& str) {
    try {
        return std::stod(str);
    } catch (...) {
        return std::nullopt;
    }
}

static const std::string BASE64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const std::vector<uint8_t>& data) {
    std::string result;
    int i = 0;
    int j = 0;
    uint8_t array3[3];
    uint8_t array4[4];

    for (size_t n = 0; n < data.size(); ++n) {
        array3[i++] = data[n];
        if (i == 3) {
            array4[0] = (array3[0] & 0xfc) >> 2;
            array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
            array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);
            array4[3] = array3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                result += BASE64_CHARS[array4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            array3[j] = '\0';

        array4[0] = (array3[0] & 0xfc) >> 2;
        array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
        array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++)
            result += BASE64_CHARS[array4[j]];

        while (i++ < 3)
            result += '=';
    }

    return result;
}

std::string base64Encode(const std::string& str) {
    return base64Encode(std::vector<uint8_t>(str.begin(), str.end()));
}

std::vector<uint8_t> base64Decode(const std::string& encoded) {
    std::vector<uint8_t> result;

    int i = 0;
    int in_len = encoded.size();
    uint8_t array4[4], array3[3];

    for (int n = 0; n < in_len && encoded[n] != '='; ++n) {
        size_t pos = BASE64_CHARS.find(encoded[n]);
        if (pos == std::string::npos) continue;

        array4[i++] = static_cast<uint8_t>(pos);
        if (i == 4) {
            array3[0] = (array4[0] << 2) + ((array4[1] & 0x30) >> 4);
            array3[1] = ((array4[1] & 0xf) << 4) + ((array4[2] & 0x3c) >> 2);
            array3[2] = ((array4[2] & 0x3) << 6) + array4[3];

            for (i = 0; i < 3; i++)
                result.push_back(array3[i]);
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 4; j++)
            array4[j] = 0;

        array3[0] = (array4[0] << 2) + ((array4[1] & 0x30) >> 4);
        array3[1] = ((array4[1] & 0xf) << 4) + ((array4[2] & 0x3c) >> 2);

        for (int j = 0; j < i - 1; j++)
            result.push_back(array3[j]);
    }

    return result;
}

std::string base64DecodeString(const std::string& encoded) {
    auto decoded = base64Decode(encoded);
    return std::string(decoded.begin(), decoded.end());
}

std::string escapeHtml(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    for (char c : str) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += c;
        }
    }

    return result;
}

std::string unescapeHtml(const std::string& str) {
    std::string result = str;
    result = replaceAll(result, "&amp;", "&");
    result = replaceAll(result, "&lt;", "<");
    result = replaceAll(result, "&gt;", ">");
    result = replaceAll(result, "&quot;", "\"");
    result = replaceAll(result, "&#39;", "'");
    return result;
}

std::string escapeXml(const std::string& str) {
    return escapeHtml(str);
}

std::string stripHtmlTags(const std::string& html) {
    std::regex tagRegex("<[^>]*>");
    return std::regex_replace(html, tagRegex, "");
}

std::string escapeJson(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
        }
    }

    return result;
}

std::string generateUuid() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;

    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);

    return ss.str();
}

std::string md5(const std::string& str) {
    // Simplified hash - use proper MD5 in production
    size_t hash = std::hash<std::string>{}(str);
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

std::string sha256(const std::string& str) {
    // Placeholder - use proper SHA256 in production
    return md5(str);
}

int levenshteinDistance(const std::string& a, const std::string& b) {
    size_t m = a.size();
    size_t n = b.size();

    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

    for (size_t i = 0; i <= m; i++) dp[i][0] = i;
    for (size_t j = 0; j <= n; j++) dp[0][j] = j;

    for (size_t i = 1; i <= m; i++) {
        for (size_t j = 1; j <= n; j++) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({
                dp[i - 1][j] + 1,
                dp[i][j - 1] + 1,
                dp[i - 1][j - 1] + cost
            });
        }
    }

    return dp[m][n];
}

std::string slugify(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    for (char c : str) {
        if (std::isalnum(c)) {
            result += std::tolower(c);
        } else if (c == ' ' || c == '-' || c == '_') {
            if (!result.empty() && result.back() != '-') {
                result += '-';
            }
        }
    }

    // Remove trailing dash
    while (!result.empty() && result.back() == '-') {
        result.pop_back();
    }

    return result;
}

std::vector<std::string> wordWrap(const std::string& text, size_t maxWidth) {
    std::vector<std::string> result;
    std::istringstream words(text);
    std::string word;
    std::string line;

    while (words >> word) {
        if (line.empty()) {
            line = word;
        } else if (line.size() + 1 + word.size() <= maxWidth) {
            line += " " + word;
        } else {
            result.push_back(line);
            line = word;
        }
    }

    if (!line.empty()) {
        result.push_back(line);
    }

    return result;
}

} // namespace string_utils
} // namespace soda
