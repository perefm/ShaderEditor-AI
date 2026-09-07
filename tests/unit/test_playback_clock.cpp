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

TEST_CASE("playback clock resets elapsed time when section duration is reached") {
    shadereditor::PlaybackClockState clock;
    clock.setSectionDurationSeconds(2.0F);

    clock.advance(1.5F);
    REQUIRE(clock.elapsedSeconds() == 1.5F);
    clock.advance(0.5F);
    REQUIRE(clock.elapsedSeconds() == 0.0F);
}

TEST_CASE("playback clock resets elapsed time when section duration is shortened below t") {
    shadereditor::PlaybackClockState clock;
    clock.setSectionDurationSeconds(5.0F);
    clock.advance(4.0F);

    clock.setSectionDurationSeconds(3.0F);

    REQUIRE(clock.elapsedSeconds() == 0.0F);
    REQUIRE(clock.sectionDurationSeconds() == 3.0F);
}

TEST_CASE("playback clock clamps section duration to a value greater than one second") {
    shadereditor::PlaybackClockState clock;

    clock.setSectionDurationSeconds(0.0F);
    REQUIRE(clock.sectionDurationSeconds() > 1.0F);
    clock.setSectionDurationSeconds(1.0F);
    REQUIRE(clock.sectionDurationSeconds() > 1.0F);
    clock.setSectionDurationSeconds(-10.0F);
    REQUIRE(clock.sectionDurationSeconds() > 1.0F);
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
    clock.advance(0.75F);
    // 120 bpm = 2 beats/second; 0.75 seconds is 1.5 beats, so the normalized phase is 0.5.
    REQUIRE(clock.beat() == 0.5F);
    REQUIRE(clock.beat() >= 0.0F);
    REQUIRE(clock.beat() < 1.0F);
}

TEST_CASE("playback clock wraps beat phase at every beat boundary") {
    shadereditor::PlaybackClockState clock;
    clock.setBpm(120.0F);

    // At 120 BPM, each beat lasts 0.5 seconds. Exact boundaries wrap to zero.
    clock.advance(0.5F);
    REQUIRE(clock.beat() == 0.0F);
    clock.advance(0.25F);
    REQUIRE(clock.beat() == 0.5F);
    clock.advance(0.5F);
    REQUIRE(clock.beat() == 0.5F);
}

TEST_CASE("playback clock beat is held at zero for non-positive bpm") {
    shadereditor::PlaybackClockState clock;
    clock.advance(10.0F);
    clock.setBpm(0.0F);
    REQUIRE(clock.beat() == 0.0F);
    clock.setBpm(-10.0F);
    REQUIRE(clock.beat() == 0.0F);
}
