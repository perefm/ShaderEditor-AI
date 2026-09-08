#pragma once

#include "rendering/models/ModelDocument.h"

#include <glm/mat4x4.hpp>

#include <string>
#include <vector>

namespace shadereditor {
// How a clip behaves once playback time passes its duration.
enum class AnimationLoopMode {
    Loop,  // wrap time back to the clip start (Phoenix's default playback behavior)
    Hold,  // clamp to the final keyframe and stay there
};

// Normalizes raw playback time into a clip-local time according to `mode`. Exposed (and unit
// tested) separately from sampling because both bone transforms and animated cameras must derive
// their time exactly the same way or they would visibly desynchronize.
// A non-positive duration always yields 0, so single-key/zero-length clips never divide by zero.
[[nodiscard]] float normalizeAnimationTime(float elapsedSeconds, float durationSeconds, AnimationLoopMode mode);

// Computes per-frame keyframe-driven transforms for an imported model: skinned bone matrices
// ("gBones") and the world transform of arbitrary scene nodes (used to place animated cameras).
// Stateless: every call recomputes the full result from the given elapsed time, so callers do
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
    [[nodiscard]] std::vector<glm::mat4> boneTransforms(const ModelDocument& document,
                                                        float elapsedSeconds,
                                                        int animationIndex = 0,
                                                        AnimationLoopMode loopMode = AnimationLoopMode::Loop) const;

    // World (model-space) transform of the named scene node at the given time, with the active
    // clip's channels applied along its whole parent chain. Returns identity when the node does
    // not exist, so callers can use the result unconditionally. This is the same hierarchy walk
    // boneTransforms() performs, exposed so a camera node - which usually has no bone - can be
    // placed by the very same keyframe data.
    [[nodiscard]] glm::mat4 nodeWorldTransform(const ModelDocument& document,
                                               const std::string& nodeName,
                                               float elapsedSeconds,
                                               int animationIndex = 0,
                                               AnimationLoopMode loopMode = AnimationLoopMode::Loop) const;

    // World transforms for *every* scene node at the given time, indexed by ModelDocument::
    // sceneNodes position. Callers that need more than one node (the renderer needs one per mesh)
    // must use this instead of calling nodeWorldTransform in a loop: the hierarchy walk costs
    // O(nodes) and allocates, so doing it per mesh is O(meshes * nodes) per frame, which dominates
    // frame time on scenes with many meshes.
    [[nodiscard]] std::vector<glm::mat4> nodeWorldTransforms(const ModelDocument& document,
                                                             float elapsedSeconds,
                                                             int animationIndex = 0,
                                                             AnimationLoopMode loopMode = AnimationLoopMode::Loop) const;
};
}  // namespace shadereditor
