#include "app/workspace/PreviewInteractionState.h"

#include <algorithm>
#include <cmath>

namespace shadereditor {
void PreviewInteractionState::orbit(const glm::vec2& delta) {
    orbitAngles += delta;
    // Clamp vertical orbit to avoid flipping the preview upside down.
    orbitAngles.y = std::clamp(orbitAngles.y, -1.4F, 1.4F);
}

// Screen-space pan follows the cursor movement in the preview.
void PreviewInteractionState::pan(const glm::vec2& delta) { panOffset += glm::vec3(-delta.x, delta.y, 0.0F); }

void PreviewInteractionState::zoom(float wheelDelta) {
    cameraDistance = std::clamp(cameraDistance * std::exp(-wheelDelta * 0.15F), 1.25F, 12.0F);
}

void PreviewInteractionState::reset() {
    orbitAngles = glm::vec2(0.0F);
    panOffset = glm::vec3(0.0F);
    cameraDistance = 4.0F;
}
}  // namespace shadereditor
