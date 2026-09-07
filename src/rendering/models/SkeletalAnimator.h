#pragma once

#include "rendering/models/ModelDocument.h"

#include <glm/mat4x4.hpp>

#include <vector>

namespace shadereditor {
// Computes the current per-frame bone transforms ("gBones") for an imported, skinned model.
// Stateless: every call recomputes the full array from the given elapsed time, so callers do
// not need to track animation progress themselves (the playback clock is the single source of
// truth for "where we are" in time, per PlaybackClockState).
class SkeletalAnimator {
  public:
    // Returns one glm::mat4 per bone (size == document.boneCount), in bone-index order, ready
    // to upload as the "gBones" uniform array. Returns identity matrices when the model has no
    // skeleton or no animation clips, so callers can always upload a valid (if static) pose.
    // animationIndex selects which of document.animations to sample; a negative or out-of-range
    // index falls back to the bind pose (identity bone transforms) rather than clamping to clip 0,
    // so callers can explicitly request "no animation".
    [[nodiscard]] std::vector<glm::mat4> boneTransforms(const ModelDocument& document, float elapsedSeconds, int animationIndex = 0) const;
};
}  // namespace shadereditor
