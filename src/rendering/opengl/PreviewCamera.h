#pragma once

#include "app/workspace/PreviewInteractionState.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace shadereditor {
// Builds the preview camera matrices from orbit/pan interaction state.
// view() and projection() are exposed separately because Phoenix shaders declare "view" and
// "projection" as independent engine uniforms; viewProjection() (and therefore "MVP") is derived
// from them so the combined matrix can never disagree with the individual ones.
class PreviewCamera {
  public:
    [[nodiscard]] glm::mat4 view(const PreviewInteractionState& interactionState) const;
    [[nodiscard]] glm::mat4 projection(const PreviewInteractionState& interactionState, float aspectRatio) const;
    [[nodiscard]] glm::mat4 viewProjection(const PreviewInteractionState& interactionState, float aspectRatio) const;
    // Base "model" factor for preview geometry. Orbit and pan are camera-space operations, so this
    // is identity for the free camera and objects keep the transform authored in the file; the
    // renderer multiplies it by each mesh's animated scene-node transform.
    [[nodiscard]] glm::mat4 modelMatrix(const PreviewInteractionState& interactionState) const;
    glm::vec3 position(const PreviewInteractionState& interactionState) const;
};
}  // namespace shadereditor
