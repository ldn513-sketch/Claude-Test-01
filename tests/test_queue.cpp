/*
 * SODA Player - Queue Tests
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <gtest/gtest.h>
#include <soda/queue.hpp>

using namespace soda;

class QueueTest : public ::testing::Test {
protected:
    Queue queue;

    TrackInfo createTrack(const std::string& id, const std::string& title) {
        TrackInfo track;
        track.id = id;
        track.title = title;
        track.artist = "Test Artist";
        return track;
    }
};

TEST_F(QueueTest, EmptyQueue) {
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_EQ(queue.size(), 0);
    EXPECT_FALSE(queue.current().has_value());
}

TEST_F(QueueTest, AddTrack) {
    queue.add(createTrack("1", "Track 1"));
    EXPECT_FALSE(queue.isEmpty());
    EXPECT_EQ(queue.size(), 1);
}

TEST_F(QueueTest, Current) {
    queue.add(createTrack("1", "Track 1"));
    auto current = queue.current();
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->id, "1");
    EXPECT_EQ(current->title, "Track 1");
}

TEST_F(QueueTest, Navigation) {
    queue.add(createTrack("1", "Track 1"));
    queue.add(createTrack("2", "Track 2"));
    queue.add(createTrack("3", "Track 3"));

    EXPECT_EQ(queue.currentIndex(), 0);

    auto next = queue.next();
    ASSERT_TRUE(next.has_value());
    EXPECT_EQ(next->id, "2");
    EXPECT_EQ(queue.currentIndex(), 1);

    next = queue.next();
    EXPECT_EQ(next->id, "3");
    EXPECT_EQ(queue.currentIndex(), 2);

    auto prev = queue.previous();
    ASSERT_TRUE(prev.has_value());
    EXPECT_EQ(prev->id, "2");
}

TEST_F(QueueTest, JumpTo) {
    queue.add(createTrack("1", "Track 1"));
    queue.add(createTrack("2", "Track 2"));
    queue.add(createTrack("3", "Track 3"));

    queue.jumpTo(2);
    EXPECT_EQ(queue.currentIndex(), 2);
    EXPECT_EQ(queue.current()->id, "3");
}

TEST_F(QueueTest, Remove) {
    queue.add(createTrack("1", "Track 1"));
    queue.add(createTrack("2", "Track 2"));
    queue.add(createTrack("3", "Track 3"));

    queue.remove(1);
    EXPECT_EQ(queue.size(), 2);

    auto tracks = queue.tracks();
    EXPECT_EQ(tracks[0].id, "1");
    EXPECT_EQ(tracks[1].id, "3");
}

TEST_F(QueueTest, Clear) {
    queue.add(createTrack("1", "Track 1"));
    queue.add(createTrack("2", "Track 2"));

    queue.clear();
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_EQ(queue.size(), 0);
}

TEST_F(QueueTest, Move) {
    queue.add(createTrack("1", "Track 1"));
    queue.add(createTrack("2", "Track 2"));
    queue.add(createTrack("3", "Track 3"));

    queue.move(0, 2);
    auto tracks = queue.tracks();
    EXPECT_EQ(tracks[0].id, "2");
    EXPECT_EQ(tracks[1].id, "3");
    EXPECT_EQ(tracks[2].id, "1");
}

TEST_F(QueueTest, Shuffle) {
    queue.add(createTrack("1", "Track 1"));
    queue.add(createTrack("2", "Track 2"));
    queue.add(createTrack("3", "Track 3"));

    EXPECT_FALSE(queue.isShuffled());
    queue.shuffle();
    EXPECT_TRUE(queue.isShuffled());
    queue.unshuffle();
    EXPECT_FALSE(queue.isShuffled());
}
