#pragma once

#include "app/workspace/PreviewInteractionState.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace shadereditor {
// Builds the preview camera matrices from orbit/pan interaction state.
class PreviewCamera {
  public:
    [[nodiscard]] glm::mat4 viewProjection(const PreviewInteractionState& interactionState, float aspectRatio) const;
    glm::vec3 position(const PreviewInteractionState& interactionState) const;
};
}  // namespace shadereditor
