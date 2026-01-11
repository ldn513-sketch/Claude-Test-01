/*
 * SODA Player - Metadata Reader Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/metadata_reader.hpp>
#include <soda/audio_decoder.hpp>
#include <soda/string_utils.hpp>
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/opusfile.h>
#include <taglib/mp4file.h>
#include <fstream>

namespace soda {

MetadataReader::MetadataReader() = default;
MetadataReader::~MetadataReader() = default;

Result<TrackInfo> MetadataReader::read(const Path& filePath) {
    if (!std::filesystem::exists(filePath)) {
        return Result<TrackInfo>("File not found: " + filePath.string());
    }

    TrackInfo track;
    track.id = generateTrackId(filePath);
    track.filePath = filePath;
    track.format = AudioDecoder::detectFormat(filePath);
    track.source = SourceType::Local;

    TagLib::FileRef file(filePath.c_str());
    if (file.isNull() || !file.tag()) {
        // Return track with just filename as title
        track.title = filePath.stem().string();
        return Result<TrackInfo>(track);
    }

    TagLib::Tag* tag = file.tag();

    track.title = tag->title().toCString(true);
    track.artist = tag->artist().toCString(true);
    track.album = tag->album().toCString(true);
    track.genre = tag->genre().toCString(true);
    track.year = tag->year();
    track.trackNumber = tag->track();

    // Fallback to filename if title is empty
    if (track.title.empty()) {
        track.title = filePath.stem().string();
    }

    // Audio properties
    if (file.audioProperties()) {
        TagLib::AudioProperties* props = file.audioProperties();
        track.duration = Duration(props->lengthInMilliseconds());
        track.bitrate = props->bitrate();
        track.sampleRate = props->sampleRate();
        track.channels = props->channels();
    }

    return Result<TrackInfo>(track);
}

std::optional<std::string> MetadataReader::readTitle(const Path& filePath) {
    TagLib::FileRef file(filePath.c_str());
    if (file.isNull() || !file.tag()) return std::nullopt;
    return std::string(file.tag()->title().toCString(true));
}

std::optional<std::string> MetadataReader::readArtist(const Path& filePath) {
    TagLib::FileRef file(filePath.c_str());
    if (file.isNull() || !file.tag()) return std::nullopt;
    return std::string(file.tag()->artist().toCString(true));
}

std::optional<std::string> MetadataReader::readAlbum(const Path& filePath) {
    TagLib::FileRef file(filePath.c_str());
    if (file.isNull() || !file.tag()) return std::nullopt;
    return std::string(file.tag()->album().toCString(true));
}

std::optional<int> MetadataReader::readYear(const Path& filePath) {
    TagLib::FileRef file(filePath.c_str());
    if (file.isNull() || !file.tag()) return std::nullopt;
    int year = file.tag()->year();
    return year > 0 ? std::optional<int>(year) : std::nullopt;
}

std::optional<int> MetadataReader::readTrackNumber(const Path& filePath) {
    TagLib::FileRef file(filePath.c_str());
    if (file.isNull() || !file.tag()) return std::nullopt;
    int track = file.tag()->track();
    return track > 0 ? std::optional<int>(track) : std::nullopt;
}

std::optional<std::string> MetadataReader::readGenre(const Path& filePath) {
    TagLib::FileRef file(filePath.c_str());
    if (file.isNull() || !file.tag()) return std::nullopt;
    return std::string(file.tag()->genre().toCString(true));
}

std::optional<Duration> MetadataReader::readDuration(const Path& filePath) {
    TagLib::FileRef file(filePath.c_str());
    if (file.isNull() || !file.audioProperties()) return std::nullopt;
    return Duration(file.audioProperties()->lengthInMilliseconds());
}

std::optional<Artwork> MetadataReader::readArtwork(const Path& filePath) {
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".mp3") {
        TagLib::MPEG::File file(filePath.c_str());
        if (!file.isValid()) return std::nullopt;

        TagLib::ID3v2::Tag* tag = file.ID3v2Tag();
        if (!tag) return std::nullopt;

        auto frames = tag->frameListMap()["APIC"];
        if (frames.isEmpty()) return std::nullopt;

        auto* frame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frames.front());

        Artwork artwork;
        artwork.mimeType = frame->mimeType().toCString();
        const auto& data = frame->picture();
        artwork.data.assign(data.begin(), data.end());

        return artwork;
    }

    if (ext == ".flac") {
        TagLib::FLAC::File file(filePath.c_str());
        if (!file.isValid()) return std::nullopt;

        auto pictures = file.pictureList();
        if (pictures.isEmpty()) return std::nullopt;

        auto* picture = pictures.front();

        Artwork artwork;
        artwork.mimeType = picture->mimeType().toCString();
        const auto& data = picture->data();
        artwork.data.assign(data.begin(), data.end());
        artwork.width = picture->width();
        artwork.height = picture->height();

        return artwork;
    }

    if (ext == ".m4a" || ext == ".mp4") {
        TagLib::MP4::File file(filePath.c_str());
        if (!file.isValid()) return std::nullopt;

        TagLib::MP4::Tag* tag = file.tag();
        if (!tag) return std::nullopt;

        if (tag->contains("covr")) {
            auto coverList = tag->item("covr").toCoverArtList();
            if (!coverList.isEmpty()) {
                auto cover = coverList.front();

                Artwork artwork;
                switch (cover.format()) {
                    case TagLib::MP4::CoverArt::JPEG: artwork.mimeType = "image/jpeg"; break;
                    case TagLib::MP4::CoverArt::PNG: artwork.mimeType = "image/png"; break;
                    default: artwork.mimeType = "image/unknown";
                }
                const auto& data = cover.data();
                artwork.data.assign(data.begin(), data.end());

                return artwork;
            }
        }
    }

    return std::nullopt;
}

Result<void> MetadataReader::extractArtwork(const Path& filePath, const Path& outputPath) {
    auto artwork = readArtwork(filePath);
    if (!artwork) {
        return Result<void>("No artwork found");
    }

    std::ofstream file(outputPath, std::ios::binary);
    if (!file) {
        return Result<void>("Failed to open output file");
    }

    file.write(reinterpret_cast<const char*>(artwork->data.data()), artwork->data.size());
    return Result<void>();
}

Result<MetadataReader::AudioProperties> MetadataReader::readAudioProperties(const Path& filePath) {
    TagLib::FileRef file(filePath.c_str());
    if (file.isNull() || !file.audioProperties()) {
        return Result<AudioProperties>("Failed to read audio properties");
    }

    TagLib::AudioProperties* props = file.audioProperties();

    AudioProperties result;
    result.sampleRate = props->sampleRate();
    result.channels = props->channels();
    result.bitrate = props->bitrate();
    result.duration = Duration(props->lengthInMilliseconds());
    result.format = AudioDecoder::detectFormat(filePath);

    return Result<AudioProperties>(result);
}

Result<void> MetadataReader::write(const Path& filePath, const TrackInfo& info) {
    TagLib::FileRef file(filePath.c_str());
    if (file.isNull() || !file.tag()) {
        return Result<void>("Failed to open file for writing");
    }

    TagLib::Tag* tag = file.tag();

    if (!info.title.empty()) {
        tag->setTitle(TagLib::String(info.title, TagLib::String::UTF8));
    }
    if (!info.artist.empty()) {
        tag->setArtist(TagLib::String(info.artist, TagLib::String::UTF8));
    }
    if (!info.album.empty()) {
        tag->setAlbum(TagLib::String(info.album, TagLib::String::UTF8));
    }
    if (!info.genre.empty()) {
        tag->setGenre(TagLib::String(info.genre, TagLib::String::UTF8));
    }
    if (info.year > 0) {
        tag->setYear(info.year);
    }
    if (info.trackNumber > 0) {
        tag->setTrack(info.trackNumber);
    }

    if (!file.save()) {
        return Result<void>("Failed to save file");
    }

    return Result<void>();
}

Result<void> MetadataReader::writeArtwork(const Path& filePath, const Path& artworkPath) {
    auto data = soda::file_utils::readBinaryFile(artworkPath);
    if (!data.ok()) {
        return Result<void>(data.error());
    }

    Artwork artwork;
    artwork.data = data.value();

    std::string ext = artworkPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".jpg" || ext == ".jpeg") {
        artwork.mimeType = "image/jpeg";
    } else if (ext == ".png") {
        artwork.mimeType = "image/png";
    } else {
        artwork.mimeType = "image/unknown";
    }

    return writeArtwork(filePath, artwork);
}

Result<void> MetadataReader::writeArtwork(const Path& filePath, const Artwork& artwork) {
    // Implementation would depend on file format
    // This is a simplified version
    return Result<void>("Artwork writing not yet implemented");
}

bool MetadataReader::isSupported(const Path& filePath) {
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    static const std::vector<std::string> supported = {
        ".mp3", ".m4a", ".flac", ".ogg", ".opus", ".wav"
    };

    return std::find(supported.begin(), supported.end(), ext) != supported.end();
}

std::vector<std::string> MetadataReader::supportedExtensions() {
    return {".mp3", ".m4a", ".flac", ".ogg", ".opus", ".wav"};
}

std::string MetadataReader::generateTrackId(const Path& filePath) {
    return "local:" + string_utils::md5(std::filesystem::absolute(filePath).string());
}

} // namespace soda
