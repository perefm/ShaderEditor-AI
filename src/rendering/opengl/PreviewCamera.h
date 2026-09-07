#pragma once

#include "app/workspace/PreviewInteractionState.h"

#include <glm/mat4x4.hpp>

namespace shadereditor {
// Builds the preview camera matrices from orbit/pan interaction state.
class PreviewCamera {
  public:
    [[nodiscard]] glm::mat4 viewProjection(const PreviewInteractionState& interactionState, float aspectRatio) const;
};
}  // namespace shadereditor
