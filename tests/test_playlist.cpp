/*
 * SODA Player - Playlist Tests
 * Copyright (C) 2026 SODA Project Contributors
 * License: GPL-3.0-or-later
 */

#include <gtest/gtest.h>
#include <soda/playlist.hpp>

using namespace soda;

TEST(PlaylistTest, CreatePlaylist) {
    Playlist playlist("My Playlist");
    EXPECT_EQ(playlist.name(), "My Playlist");
    EXPECT_TRUE(playlist.isEmpty());
    EXPECT_EQ(playlist.size(), 0);
}

TEST(PlaylistTest, AddTracks) {
    Playlist playlist("Test");
    playlist.addTrack("track1");
    playlist.addTrack("track2");

    EXPECT_EQ(playlist.size(), 2);
    EXPECT_TRUE(playlist.contains("track1"));
    EXPECT_TRUE(playlist.contains("track2"));
    EXPECT_FALSE(playlist.contains("track3"));
}

TEST(PlaylistTest, RemoveTrack) {
    Playlist playlist("Test");
    playlist.addTrack("track1");
    playlist.addTrack("track2");
    playlist.addTrack("track3");

    playlist.removeTrack("track2");
    EXPECT_EQ(playlist.size(), 2);
    EXPECT_FALSE(playlist.contains("track2"));
}

TEST(PlaylistTest, RemoveTrackAt) {
    Playlist playlist("Test");
    playlist.addTrack("track1");
    playlist.addTrack("track2");
    playlist.addTrack("track3");

    playlist.removeTrackAt(1);
    EXPECT_EQ(playlist.size(), 2);

    const auto& tracks = playlist.trackIds();
    EXPECT_EQ(tracks[0], "track1");
    EXPECT_EQ(tracks[1], "track3");
}

TEST(PlaylistTest, MoveTrack) {
    Playlist playlist("Test");
    playlist.addTrack("track1");
    playlist.addTrack("track2");
    playlist.addTrack("track3");

    playlist.moveTrack(0, 2);
    const auto& tracks = playlist.trackIds();
    EXPECT_EQ(tracks[0], "track2");
    EXPECT_EQ(tracks[1], "track3");
    EXPECT_EQ(tracks[2], "track1");
}

TEST(PlaylistTest, Clear) {
    Playlist playlist("Test");
    playlist.addTrack("track1");
    playlist.addTrack("track2");

    playlist.clear();
    EXPECT_TRUE(playlist.isEmpty());
    EXPECT_EQ(playlist.size(), 0);
}

TEST(PlaylistTest, SetName) {
    Playlist playlist("Original");
    playlist.setName("Renamed");
    EXPECT_EQ(playlist.name(), "Renamed");
}

TEST(PlaylistTest, SetDescription) {
    Playlist playlist("Test");
    playlist.setDescription("A test playlist");
    EXPECT_EQ(playlist.description(), "A test playlist");
}

TEST(PlaylistTest, HasUniqueId) {
    Playlist playlist1("Playlist 1");
    Playlist playlist2("Playlist 2");
    EXPECT_NE(playlist1.id(), playlist2.id());
}
