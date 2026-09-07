#include "catch2/catch_test_macros.hpp"

#include "app/workspace/PlaybackClockState.h"

TEST_CASE("playback clock defaults to playing from zero") {
    shadereditor::PlaybackClockState clock;
    REQUIRE(clock.isPlaying());
    REQUIRE(clock.elapsedSeconds() == 0.0F);
}

TEST_CASE("playback clock advances elapsed time only while playing") {
    shadereditor::PlaybackClockState clock;
    clock.advance(1.5F);
    REQUIRE(clock.elapsedSeconds() == 1.5F);

    clock.pause();
    clock.advance(2.0F);
    // Paused: elapsed time must not move even though advance() was called.
    REQUIRE(clock.elapsedSeconds() == 1.5F);

    clock.play();
    clock.advance(0.5F);
    REQUIRE(clock.elapsedSeconds() == 2.0F);
}

TEST_CASE("playback clock advance ignores non-positive deltas") {
    shadereditor::PlaybackClockState clock;
    clock.advance(1.0F);
    clock.advance(-5.0F);
    clock.advance(0.0F);
    REQUIRE(clock.elapsedSeconds() == 1.0F);
}

TEST_CASE("playback clock reset zeroes elapsed time without touching play state or config") {
    shadereditor::PlaybackClockState clock;
    clock.pause();
    clock.advance(3.0F);  // no-op while paused, but exercises the call path
    clock.play();
    clock.advance(3.0F);
    clock.setSectionDurationSeconds(42.0F);
    clock.setBpm(90.0F);

    clock.reset();

    REQUIRE(clock.elapsedSeconds() == 0.0F);
    REQUIRE(clock.isPlaying());
    REQUIRE(clock.sectionDurationSeconds() == 42.0F);
    REQUIRE(clock.bpm() == 90.0F);
}

TEST_CASE("playback clock derives beat from elapsed time and bpm") {
    shadereditor::PlaybackClockState clock;
    clock.setBpm(120.0F);
    clock.advance(30.0F);
    // 120 bpm = 2 beats/second, so 30 seconds elapsed = 60 beats.
    REQUIRE(clock.beat() == 60.0F);
}

TEST_CASE("playback clock beat is held at zero for non-positive bpm") {
    shadereditor::PlaybackClockState clock;
    clock.advance(10.0F);
    clock.setBpm(0.0F);
    REQUIRE(clock.beat() == 0.0F);
    clock.setBpm(-10.0F);
    REQUIRE(clock.beat() == 0.0F);
}
