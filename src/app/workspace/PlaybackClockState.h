#pragma once

namespace shadereditor {
// Owns the Play/Pause/Reset transport and the elapsed time that feeds Phoenix's
// time-based auto-uniforms ("t", "tend", "beat"). Analogous in spirit to
// PreviewInteractionState, but for time instead of camera framing.
//
// - "t"    -> elapsedSeconds(): seconds since the clock was last reset, frozen while paused.
// - "tend" -> sectionDurationSeconds(): a user-editable "section length" value; Phoenix shaders
//             typically use it to normalize/loop time (t / tend), but this app does not impose
//             that math itself - it simply supplies the raw value the shader author chose.
// - "beat" -> beat(): derived from elapsedSeconds() and bpm(), so it can never drift out of sync.
class PlaybackClockState {
  public:
    // Starts/stops advancing elapsedSeconds(); does not touch its current value.
    void play() { isPlaying_ = true; }
    void pause() { isPlaying_ = false; }
    // Resets elapsed time back to zero without changing play state, section duration, or bpm.
    void reset() { elapsedSeconds_ = 0.0F; }

    // Advances elapsed time by one frame's delta; a no-op while paused.
    void advance(float deltaSeconds) {
        if (isPlaying_ && deltaSeconds > 0.0F) {
            elapsedSeconds_ += deltaSeconds;
        }
    }

    void setSectionDurationSeconds(float value) { sectionDurationSeconds_ = value; }
    void setBpm(float value) { bpm_ = value; }

    [[nodiscard]] bool isPlaying() const { return isPlaying_; }
    [[nodiscard]] float elapsedSeconds() const { return elapsedSeconds_; }
    [[nodiscard]] float sectionDurationSeconds() const { return sectionDurationSeconds_; }
    [[nodiscard]] float bpm() const { return bpm_; }

    // "beat" counts musical beats elapsed since t=0. A non-positive BPM has no meaningful beat
    // rate, so the value is held at 0 instead of producing NaN/Inf (FR-010).
    [[nodiscard]] float beat() const { return bpm_ <= 0.0F ? 0.0F : elapsedSeconds_ * bpm_ / 60.0F; }

  private:
    bool isPlaying_ {true};
    float elapsedSeconds_ {0.0F};
    float sectionDurationSeconds_ {60.0F};
    float bpm_ {120.0F};
};
}  // namespace shadereditor
