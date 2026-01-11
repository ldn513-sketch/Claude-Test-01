/*
 * SODA Player - Metadata Reader
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_METADATA_READER_HPP
#define SODA_METADATA_READER_HPP

#include "soda.hpp"

namespace soda {

// Embedded artwork
struct Artwork {
    std::vector<uint8_t> data;
    std::string mimeType;
    int width = 0;
    int height = 0;
};

class MetadataReader {
public:
    MetadataReader();
    ~MetadataReader();

    // Read all metadata from a file
    Result<TrackInfo> read(const Path& filePath);

    // Read specific fields
    std::optional<std::string> readTitle(const Path& filePath);
    std::optional<std::string> readArtist(const Path& filePath);
    std::optional<std::string> readAlbum(const Path& filePath);
    std::optional<int> readYear(const Path& filePath);
    std::optional<int> readTrackNumber(const Path& filePath);
    std::optional<std::string> readGenre(const Path& filePath);
    std::optional<Duration> readDuration(const Path& filePath);

    // Artwork
    std::optional<Artwork> readArtwork(const Path& filePath);
    Result<void> extractArtwork(const Path& filePath, const Path& outputPath);

    // Audio properties (without metadata)
    struct AudioProperties {
        int sampleRate = 0;
        int channels = 0;
        int bitrate = 0;
        Duration duration{0};
        AudioFormat format = AudioFormat::Unknown;
    };
    Result<AudioProperties> readAudioProperties(const Path& filePath);

    // Write metadata
    Result<void> write(const Path& filePath, const TrackInfo& info);
    Result<void> writeArtwork(const Path& filePath, const Path& artworkPath);
    Result<void> writeArtwork(const Path& filePath, const Artwork& artwork);

    // Supported formats
    static bool isSupported(const Path& filePath);
    static std::vector<std::string> supportedExtensions();

private:
    Result<TrackInfo> readMp3(const Path& filePath);
    Result<TrackInfo> readFlac(const Path& filePath);
    Result<TrackInfo> readOgg(const Path& filePath);
    Result<TrackInfo> readM4a(const Path& filePath);
    Result<TrackInfo> readOpus(const Path& filePath);
    Result<TrackInfo> readWav(const Path& filePath);

    std::string generateTrackId(const Path& filePath);
};

} // namespace soda

#endif // SODA_METADATA_READER_HPP
