#pragma once

#include <cmath>

namespace shadereditor {
// Owns the Play/Pause/Reset transport and the elapsed time that feeds Phoenix's
// time-based auto-uniforms ("t", "tend", "beat"). Analogous in spirit to
// PreviewInteractionState, but for time instead of camera framing.
//
// - "t"    -> elapsedSeconds(): seconds since the clock was last reset, frozen while paused.
// - "tend" -> sectionDurationSeconds(): a user-editable section length greater than 1 second.
//             Reaching or changing to a duration below the current t resets elapsed time to 0.
// - "beat" -> beat(): derived from elapsedSeconds() and bpm(), so it can never drift out of sync.
class PlaybackClockState {
  public:
    static constexpr float kMinimumSectionDurationSeconds {1.0001F};

    // Starts/stops advancing elapsedSeconds(); does not touch its current value.
    void play() { isPlaying_ = true; }
    void pause() { isPlaying_ = false; }
    // Resets elapsed time back to zero without changing play state, section duration, or bpm.
    void reset() { elapsedSeconds_ = 0.0F; }

    // Advances elapsed time by one frame's delta; a no-op while paused.
    void advance(float deltaSeconds) {
        if (isPlaying_ && deltaSeconds > 0.0F) {
            elapsedSeconds_ += deltaSeconds;
            if (elapsedSeconds_ >= sectionDurationSeconds_) {
                elapsedSeconds_ = 0.0F;
            }
        }
    }

    void setSectionDurationSeconds(float value) {
        sectionDurationSeconds_ = value > kMinimumSectionDurationSeconds ? value : kMinimumSectionDurationSeconds;
        if (elapsedSeconds_ >= sectionDurationSeconds_) {
            elapsedSeconds_ = 0.0F;
        }
    }
    void setBpm(float value) { bpm_ = value; }

    [[nodiscard]] bool isPlaying() const { return isPlaying_; }
    [[nodiscard]] float elapsedSeconds() const { return elapsedSeconds_; }
    [[nodiscard]] float sectionDurationSeconds() const { return sectionDurationSeconds_; }
    [[nodiscard]] float bpm() const { return bpm_; }

    // "beat" is the normalized phase of the current musical beat: 0 at the beat boundary and
    // approaching 1 immediately before the next one. Wrapping it keeps the Phoenix uniform in
    // the documented [0, 1) range instead of exposing an ever-growing beat count (FR-010).
    [[nodiscard]] float beat() const {
        if (bpm_ <= 0.0F) {
            return 0.0F;
        }
        const float beatsElapsed = elapsedSeconds_ * bpm_ / 60.0F;
        return beatsElapsed - std::floor(beatsElapsed);
    }

  private:
    bool isPlaying_ {true};
    float elapsedSeconds_ {0.0F};
    float sectionDurationSeconds_ {60.0F};
    float bpm_ {120.0F};
};
}  // namespace shadereditor
