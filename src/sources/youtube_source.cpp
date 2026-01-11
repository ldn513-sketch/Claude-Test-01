/*
 * SODA Player - YouTube Source Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 *
 * Note: This implementation extracts audio streams from YouTube.
 * Users are responsible for ensuring their use complies with
 * applicable laws and YouTube's Terms of Service.
 */

#include <soda/youtube_source.hpp>
#include <soda/http_client.hpp>
#include <soda/event_bus.hpp>
#include <soda/string_utils.hpp>
#include <regex>
#include <sstream>

namespace soda {

YouTubeSource::YouTubeSource(EventBus& eventBus)
    : eventBus_(eventBus)
    , httpClient_(std::make_unique<HttpClient>())
{
}

YouTubeSource::~YouTubeSource() = default;

bool YouTubeSource::isAvailable() const {
    // Check if we can reach YouTube
    auto response = httpClient_->get("https://www.youtube.com");
    return response.success();
}

std::vector<SearchResult> YouTubeSource::search(const std::string& query) {
    std::vector<SearchResult> results;

    // YouTube search would require using their API or web scraping
    // This is a simplified placeholder implementation
    // In production, you'd use the YouTube Data API v3 or yt-dlp

    std::string url = "https://www.youtube.com/results?search_query=" +
                      HttpClient::urlEncode(query);

    auto response = httpClient_->get(url);
    if (!response.success()) {
        return results;
    }

    // Parse search results from HTML
    // This is a simplified regex approach - in production use proper parsing
    std::regex videoRegex(R"(/watch\?v=([a-zA-Z0-9_-]{11}))");
    std::regex titleRegex(R"("title":\{"runs":\[\{"text":"([^"]+)"\}\])");

    auto videoBegin = std::sregex_iterator(response.body.begin(), response.body.end(), videoRegex);
    auto videoEnd = std::sregex_iterator();

    std::unordered_set<std::string> seenIds;

    for (auto it = videoBegin; it != videoEnd && results.size() < 20; ++it) {
        std::string videoId = (*it)[1].str();

        if (seenIds.find(videoId) != seenIds.end()) {
            continue;
        }
        seenIds.insert(videoId);

        SearchResult result;
        result.id = "youtube:" + videoId;
        result.sourceId = videoId;
        result.source = SourceType::YouTube;
        result.title = "YouTube Video: " + videoId; // Would need to extract actual title
        result.thumbnailUrl = "https://i.ytimg.com/vi/" + videoId + "/hqdefault.jpg";
        result.isPlaylist = false;

        results.push_back(result);
    }

    return results;
}

Result<TrackInfo> YouTubeSource::getTrack(const std::string& id) {
    // Remove "youtube:" prefix if present
    std::string videoId = id;
    if (videoId.rfind("youtube:", 0) == 0) {
        videoId = videoId.substr(8);
    }

    auto infoResult = getVideoInfo(videoId);
    if (!infoResult.ok()) {
        return Result<TrackInfo>(infoResult.error());
    }

    return Result<TrackInfo>(videoInfoToTrack(infoResult.value()));
}

Result<std::string> YouTubeSource::getStreamUrl(const std::string& id) {
    std::string videoId = id;
    if (videoId.rfind("youtube:", 0) == 0) {
        videoId = videoId.substr(8);
    }

    return getAudioUrl(videoId);
}

Result<Path> YouTubeSource::download(const std::string& id, const Path& destination) {
    std::string videoId = id;
    if (videoId.rfind("youtube:", 0) == 0) {
        videoId = videoId.substr(8);
    }

    return downloadAudio(videoId, destination);
}

Result<YouTubeVideoInfo> YouTubeSource::getVideoInfo(const std::string& videoId) {
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto it = infoCache_.find(videoId);
        if (it != infoCache_.end()) {
            return Result<YouTubeVideoInfo>(it->second);
        }
    }

    return fetchVideoInfo(videoId);
}

Result<YouTubeVideoInfo> YouTubeSource::fetchVideoInfo(const std::string& videoId) {
    // Fetch video page
    std::string url = "https://www.youtube.com/watch?v=" + videoId;
    auto response = httpClient_->get(url);

    if (!response.success()) {
        return Result<YouTubeVideoInfo>("Failed to fetch video page");
    }

    YouTubeVideoInfo info;
    info.videoId = videoId;

    // Extract player response JSON
    // This is a simplified approach - production code would use yt-dlp or similar
    std::regex playerResponseRegex(R"(var ytInitialPlayerResponse = (\{.+?\});)");
    std::smatch match;

    if (std::regex_search(response.body, match, playerResponseRegex)) {
        // Parse JSON response
        // In production, use a proper JSON parser
        std::string json = match[1].str();

        // Extract basic info using regex (simplified)
        std::regex titleRegex(R"("title":"([^"]+)")");
        std::regex authorRegex(R"("author":"([^"]+)")");
        std::regex lengthRegex(R"("lengthSeconds":"(\d+)")");

        std::smatch titleMatch, authorMatch, lengthMatch;

        if (std::regex_search(json, titleMatch, titleRegex)) {
            info.title = titleMatch[1].str();
        }
        if (std::regex_search(json, authorMatch, authorRegex)) {
            info.author = authorMatch[1].str();
        }
        if (std::regex_search(json, lengthMatch, lengthRegex)) {
            info.lengthSeconds = std::stoi(lengthMatch[1].str());
        }
    }

    info.thumbnail = "https://i.ytimg.com/vi/" + videoId + "/maxresdefault.jpg";

    // Cache the result
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        infoCache_[videoId] = info;
    }

    return Result<YouTubeVideoInfo>(info);
}

Result<std::string> YouTubeSource::getAudioUrl(const std::string& videoId, const AudioOptions& options) {
    auto infoResult = getVideoInfo(videoId);
    if (!infoResult.ok()) {
        return Result<std::string>(infoResult.error());
    }

    const auto& info = infoResult.value();

    // Find best audio format
    auto format = selectBestAudioFormat(info.adaptiveFormats, options);
    if (!format) {
        format = selectBestAudioFormat(info.formats, options);
    }

    if (!format) {
        return Result<std::string>("No suitable audio format found");
    }

    return Result<std::string>(format->url);
}

std::optional<YouTubeFormat> YouTubeSource::selectBestAudioFormat(
    const std::vector<YouTubeFormat>& formats,
    const AudioOptions& options) {

    std::vector<const YouTubeFormat*> audioFormats;

    for (const auto& format : formats) {
        if (format.hasAudio && !format.hasVideo) {
            audioFormats.push_back(&format);
        }
    }

    if (audioFormats.empty()) {
        // Try formats with both audio and video
        for (const auto& format : formats) {
            if (format.hasAudio) {
                audioFormats.push_back(&format);
            }
        }
    }

    if (audioFormats.empty()) {
        return std::nullopt;
    }

    // Sort by bitrate (descending)
    std::sort(audioFormats.begin(), audioFormats.end(),
              [](const YouTubeFormat* a, const YouTubeFormat* b) {
                  return a->bitrate > b->bitrate;
              });

    // Select based on quality preference
    if (options.preferredQuality == "low") {
        return *audioFormats.back();
    } else if (options.preferredQuality == "medium") {
        return *audioFormats[audioFormats.size() / 2];
    } else {
        return *audioFormats.front();
    }
}

Result<YouTubePlaylistInfo> YouTubeSource::getPlaylistInfo(const std::string& playlistId) {
    // Playlist extraction would go here
    return Result<YouTubePlaylistInfo>("Playlist extraction not yet implemented");
}

Result<std::vector<std::string>> YouTubeSource::getPlaylistVideoIds(const std::string& playlistId) {
    return Result<std::vector<std::string>>("Playlist extraction not yet implemented");
}

Result<PlaylistInfo> YouTubeSource::importPlaylist(const std::string& playlistUrl) {
    auto playlistId = parsePlaylistId(playlistUrl);
    if (!playlistId) {
        return Result<PlaylistInfo>("Invalid playlist URL");
    }

    auto infoResult = getPlaylistInfo(*playlistId);
    if (!infoResult.ok()) {
        return Result<PlaylistInfo>(infoResult.error());
    }

    PlaylistInfo info;
    info.id = "yt-playlist:" + *playlistId;
    info.name = infoResult.value().title;
    info.description = infoResult.value().description;
    info.source = SourceType::YouTube;
    info.sourceId = *playlistId;

    return Result<PlaylistInfo>(info);
}

bool YouTubeSource::hasChapters(const std::string& videoId) {
    auto infoResult = getVideoInfo(videoId);
    if (!infoResult.ok()) {
        return false;
    }
    return !infoResult.value().chapters.empty();
}

std::vector<TrackInfo> YouTubeSource::splitByChapters(const std::string& videoId) {
    std::vector<TrackInfo> tracks;

    auto infoResult = getVideoInfo(videoId);
    if (!infoResult.ok()) {
        return tracks;
    }

    const auto& info = infoResult.value();

    for (size_t i = 0; i < info.chapters.size(); ++i) {
        const auto& chapter = info.chapters[i];

        TrackInfo track;
        track.id = "youtube:" + videoId + ":" + std::to_string(i);
        track.title = chapter.title;
        track.artist = info.author;
        track.album = info.title;
        track.trackNumber = static_cast<int>(i + 1);
        track.duration = Duration((chapter.endTimeSeconds - chapter.startTimeSeconds) * 1000);
        track.source = SourceType::YouTube;
        track.sourceId = videoId;
        track.coverUrl = info.thumbnail;

        tracks.push_back(track);
    }

    return tracks;
}

Result<Path> YouTubeSource::downloadAudio(const std::string& videoId,
                                           const Path& destination,
                                           const AudioOptions& options,
                                           ProgressCallback progress) {
    auto urlResult = getAudioUrl(videoId, options);
    if (!urlResult.ok()) {
        return Result<Path>(urlResult.error());
    }

    auto infoResult = getVideoInfo(videoId);
    std::string filename = videoId;
    if (infoResult.ok()) {
        filename = string_utils::slugify(infoResult.value().title);
    }

    Path outputPath = destination / (filename + ".opus");

    auto downloadResult = httpClient_->downloadFile(urlResult.value(), outputPath, progress);
    if (!downloadResult.ok()) {
        return Result<Path>(downloadResult.error());
    }

    return Result<Path>(outputPath);
}

std::optional<std::string> YouTubeSource::parseVideoId(const std::string& url) {
    // Match various YouTube URL formats
    std::regex patterns[] = {
        std::regex(R"((?:youtube\.com/watch\?v=|youtu\.be/)([a-zA-Z0-9_-]{11}))"),
        std::regex(R"(youtube\.com/embed/([a-zA-Z0-9_-]{11}))"),
        std::regex(R"(youtube\.com/v/([a-zA-Z0-9_-]{11}))")
    };

    for (const auto& pattern : patterns) {
        std::smatch match;
        if (std::regex_search(url, match, pattern)) {
            return match[1].str();
        }
    }

    // Check if it's already just a video ID
    if (url.length() == 11 && std::regex_match(url, std::regex(R"([a-zA-Z0-9_-]{11})"))) {
        return url;
    }

    return std::nullopt;
}

std::optional<std::string> YouTubeSource::parsePlaylistId(const std::string& url) {
    std::regex pattern(R"((?:list=)([a-zA-Z0-9_-]+))");
    std::smatch match;

    if (std::regex_search(url, match, pattern)) {
        return match[1].str();
    }

    return std::nullopt;
}

bool YouTubeSource::isYouTubeUrl(const std::string& url) {
    return url.find("youtube.com") != std::string::npos ||
           url.find("youtu.be") != std::string::npos;
}

void YouTubeSource::clearCache() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    infoCache_.clear();
}

size_t YouTubeSource::cacheSize() const {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    return infoCache_.size();
}

TrackInfo YouTubeSource::videoInfoToTrack(const YouTubeVideoInfo& info) {
    TrackInfo track;
    track.id = "youtube:" + info.videoId;
    track.title = info.title;
    track.artist = info.author;
    track.duration = Duration(info.lengthSeconds * 1000);
    track.source = SourceType::YouTube;
    track.sourceId = info.videoId;
    track.coverUrl = info.thumbnail;
    return track;
}

} // namespace soda
