#pragma once

#include "app/workspace/PreviewInteractionState.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace shadereditor {
// Builds the preview camera matrices from orbit/pan interaction state.
class PreviewCamera {
  public:
    [[nodiscard]] glm::mat4 viewProjection(const PreviewInteractionState& interactionState, float aspectRatio) const;
    // Isolated model-space rotation applied to the orbited geometry (the same "model" factor
    // baked into viewProjection()'s combined matrix), exposed separately so shaders that need
    // it standalone (e.g. to transform normals) can receive Phoenix's "model" uniform correctly.
    [[nodiscard]] glm::mat4 modelMatrix(const PreviewInteractionState& interactionState) const;
    glm::vec3 position(const PreviewInteractionState& interactionState) const;
};
}  // namespace shadereditor
