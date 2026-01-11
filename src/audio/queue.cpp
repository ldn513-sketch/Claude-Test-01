/*
 * SODA Player - Queue Implementation
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <soda/queue.hpp>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <algorithm>
#include <numeric>

namespace soda {

void Queue::add(const TrackInfo& track) {
    std::lock_guard<std::mutex> lock(mutex_);
    tracks_.push_back(track);
    if (shuffled_) {
        shuffleOrder_.push_back(tracks_.size() - 1);
    }
}

void Queue::addAll(const std::vector<TrackInfo>& tracks) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t startIndex = tracks_.size();
    tracks_.insert(tracks_.end(), tracks.begin(), tracks.end());

    if (shuffled_) {
        for (size_t i = startIndex; i < tracks_.size(); ++i) {
            shuffleOrder_.push_back(i);
        }
    }
}

void Queue::addNext(const TrackInfo& track) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t insertPos = currentIndex_ + 1;

    if (insertPos >= tracks_.size()) {
        tracks_.push_back(track);
    } else {
        tracks_.insert(tracks_.begin() + insertPos, track);
    }

    if (shuffled_) {
        updateShuffleOrder();
    }
}

void Queue::remove(size_t index) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (index >= tracks_.size()) {
        return;
    }

    tracks_.erase(tracks_.begin() + index);

    if (index < currentIndex_) {
        --currentIndex_;
    } else if (index == currentIndex_ && currentIndex_ >= tracks_.size()) {
        currentIndex_ = tracks_.empty() ? 0 : tracks_.size() - 1;
    }

    if (shuffled_) {
        updateShuffleOrder();
    }
}

void Queue::remove(const std::string& trackId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(tracks_.begin(), tracks_.end(),
                           [&trackId](const TrackInfo& t) { return t.id == trackId; });
    if (it != tracks_.end()) {
        size_t index = std::distance(tracks_.begin(), it);
        tracks_.erase(it);

        if (index < currentIndex_) {
            --currentIndex_;
        }

        if (shuffled_) {
            updateShuffleOrder();
        }
    }
}

void Queue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    tracks_.clear();
    shuffleOrder_.clear();
    history_.clear();
    currentIndex_ = 0;
}

void Queue::move(size_t fromIndex, size_t toIndex) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fromIndex >= tracks_.size() || toIndex >= tracks_.size()) {
        return;
    }

    TrackInfo track = std::move(tracks_[fromIndex]);
    tracks_.erase(tracks_.begin() + fromIndex);
    tracks_.insert(tracks_.begin() + toIndex, std::move(track));

    // Update current index
    if (fromIndex == currentIndex_) {
        currentIndex_ = toIndex;
    } else if (fromIndex < currentIndex_ && toIndex >= currentIndex_) {
        --currentIndex_;
    } else if (fromIndex > currentIndex_ && toIndex <= currentIndex_) {
        ++currentIndex_;
    }

    if (shuffled_) {
        updateShuffleOrder();
    }
}

std::optional<TrackInfo> Queue::current() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (tracks_.empty()) {
        return std::nullopt;
    }

    size_t index = shuffled_ ? shuffleOrder_[currentIndex_] : currentIndex_;
    return tracks_[index];
}

std::optional<TrackInfo> Queue::next() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (tracks_.empty()) {
        return std::nullopt;
    }

    history_.push_back(currentIndex_);

    if (currentIndex_ + 1 >= tracks_.size()) {
        return std::nullopt;
    }

    ++currentIndex_;
    size_t index = shuffled_ ? shuffleOrder_[currentIndex_] : currentIndex_;
    return tracks_[index];
}

std::optional<TrackInfo> Queue::previous() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (tracks_.empty()) {
        return std::nullopt;
    }

    // Use history if available
    if (!history_.empty()) {
        currentIndex_ = history_.back();
        history_.pop_back();
    } else if (currentIndex_ > 0) {
        --currentIndex_;
    } else {
        return std::nullopt;
    }

    size_t index = shuffled_ ? shuffleOrder_[currentIndex_] : currentIndex_;
    return tracks_[index];
}

std::optional<TrackInfo> Queue::peekNext() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (tracks_.empty() || currentIndex_ + 1 >= tracks_.size()) {
        return std::nullopt;
    }

    size_t nextIdx = currentIndex_ + 1;
    size_t index = shuffled_ ? shuffleOrder_[nextIdx] : nextIdx;
    return tracks_[index];
}

std::optional<TrackInfo> Queue::peekPrevious() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (tracks_.empty() || currentIndex_ == 0) {
        return std::nullopt;
    }

    size_t prevIdx = currentIndex_ - 1;
    size_t index = shuffled_ ? shuffleOrder_[prevIdx] : prevIdx;
    return tracks_[index];
}

void Queue::jumpTo(size_t index) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (index < tracks_.size()) {
        history_.push_back(currentIndex_);
        currentIndex_ = index;
    }
}

void Queue::jumpToId(const std::string& trackId) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (size_t i = 0; i < tracks_.size(); ++i) {
        size_t idx = shuffled_ ? shuffleOrder_[i] : i;
        if (tracks_[idx].id == trackId) {
            history_.push_back(currentIndex_);
            currentIndex_ = i;
            return;
        }
    }
}

void Queue::shuffle() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (tracks_.empty()) {
        return;
    }

    shuffleOrder_.resize(tracks_.size());
    std::iota(shuffleOrder_.begin(), shuffleOrder_.end(), 0);

    // Keep current track at current position
    size_t currentTrackIndex = shuffled_ ? shuffleOrder_[currentIndex_] : currentIndex_;

    // Fisher-Yates shuffle, but skip the current position
    for (size_t i = tracks_.size() - 1; i > 0; --i) {
        if (i == currentIndex_) continue;

        std::uniform_int_distribution<size_t> dist(0, i);
        size_t j = dist(rng_);
        if (j == currentIndex_) {
            j = (j + 1) % tracks_.size();
            if (j == currentIndex_) j = 0;
        }

        std::swap(shuffleOrder_[i], shuffleOrder_[j]);
    }

    // Ensure current track stays at current position
    auto it = std::find(shuffleOrder_.begin(), shuffleOrder_.end(), currentTrackIndex);
    if (it != shuffleOrder_.end()) {
        std::swap(*it, shuffleOrder_[currentIndex_]);
    }

    shuffled_ = true;
}

void Queue::unshuffle() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!shuffled_) {
        return;
    }

    // Find the real index of current track
    size_t realIndex = shuffleOrder_[currentIndex_];
    currentIndex_ = realIndex;
    shuffleOrder_.clear();
    shuffled_ = false;
}

void Queue::updateShuffleOrder() {
    if (tracks_.empty()) {
        shuffleOrder_.clear();
        return;
    }

    shuffleOrder_.resize(tracks_.size());
    std::iota(shuffleOrder_.begin(), shuffleOrder_.end(), 0);

    // Re-shuffle
    for (size_t i = tracks_.size() - 1; i > 0; --i) {
        std::uniform_int_distribution<size_t> dist(0, i);
        std::swap(shuffleOrder_[i], shuffleOrder_[dist(rng_)]);
    }
}

void Queue::saveToFile(const Path& path) const {
    std::lock_guard<std::mutex> lock(mutex_);

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "currentIndex" << YAML::Value << currentIndex_;
    out << YAML::Key << "shuffled" << YAML::Value << shuffled_;

    out << YAML::Key << "tracks" << YAML::Value << YAML::BeginSeq;
    for (const auto& track : tracks_) {
        out << YAML::BeginMap;
        out << YAML::Key << "id" << YAML::Value << track.id;
        out << YAML::Key << "title" << YAML::Value << track.title;
        out << YAML::Key << "artist" << YAML::Value << track.artist;
        out << YAML::Key << "album" << YAML::Value << track.album;
        out << YAML::Key << "filePath" << YAML::Value << track.filePath.string();
        out << YAML::Key << "source" << YAML::Value << static_cast<int>(track.source);
        out << YAML::Key << "sourceId" << YAML::Value << track.sourceId;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;

    std::ofstream file(path);
    file << out.c_str();
}

Queue Queue::loadFromFile(const Path& path) {
    Queue queue;

    try {
        YAML::Node node = YAML::LoadFile(path.string());

        queue.currentIndex_ = node["currentIndex"].as<size_t>(0);
        queue.shuffled_ = node["shuffled"].as<bool>(false);

        if (node["tracks"]) {
            for (const auto& trackNode : node["tracks"]) {
                TrackInfo track;
                track.id = trackNode["id"].as<std::string>();
                track.title = trackNode["title"].as<std::string>("");
                track.artist = trackNode["artist"].as<std::string>("");
                track.album = trackNode["album"].as<std::string>("");
                track.filePath = trackNode["filePath"].as<std::string>("");
                track.source = static_cast<SourceType>(trackNode["source"].as<int>(0));
                track.sourceId = trackNode["sourceId"].as<std::string>("");
                queue.tracks_.push_back(track);
            }
        }

        if (queue.shuffled_) {
            queue.updateShuffleOrder();
        }

    } catch (...) {
        // Return empty queue on error
    }

    return queue;
}

} // namespace soda
