/*
 * SODA Player - Playback Queue
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#ifndef SODA_QUEUE_HPP
#define SODA_QUEUE_HPP

#include "soda.hpp"
#include <random>

namespace soda {

class Queue {
public:
    Queue() = default;
    ~Queue() = default;

    // Queue operations
    void add(const TrackInfo& track);
    void addAll(const std::vector<TrackInfo>& tracks);
    void addNext(const TrackInfo& track);
    void remove(size_t index);
    void remove(const std::string& trackId);
    void clear();
    void move(size_t fromIndex, size_t toIndex);

    // Navigation
    std::optional<TrackInfo> current() const;
    std::optional<TrackInfo> next();
    std::optional<TrackInfo> previous();
    std::optional<TrackInfo> peekNext() const;
    std::optional<TrackInfo> peekPrevious() const;
    void jumpTo(size_t index);
    void jumpToId(const std::string& trackId);

    // State
    bool isEmpty() const { return tracks_.empty(); }
    size_t size() const { return tracks_.size(); }
    size_t currentIndex() const { return currentIndex_; }
    const std::vector<TrackInfo>& tracks() const { return tracks_; }

    // Shuffle
    void shuffle();
    void unshuffle();
    bool isShuffled() const { return shuffled_; }

    // History (for back navigation even with shuffle)
    const std::vector<size_t>& history() const { return history_; }

    // Persistence
    void saveToFile(const Path& path) const;
    static Queue loadFromFile(const Path& path);

private:
    void updateShuffleOrder();

    std::vector<TrackInfo> tracks_;
    std::vector<size_t> shuffleOrder_;
    std::vector<size_t> history_;
    size_t currentIndex_ = 0;
    bool shuffled_ = false;

    std::mt19937 rng_{std::random_device{}()};
    mutable std::mutex mutex_;
};

} // namespace soda

#endif // SODA_QUEUE_HPP
