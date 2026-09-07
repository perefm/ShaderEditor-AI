#include "app/workspace/PreviewInteractionState.h"

#include <algorithm>
#include <cmath>

namespace shadereditor {
void PreviewInteractionState::orbit(const glm::vec2& delta) {
    orbitAngles += delta;
    // Clamp vertical orbit to avoid flipping the preview upside down.
    orbitAngles.y = std::clamp(orbitAngles.y, -1.4F, 1.4F);
}

// Screen-space pan follows the cursor movement in the preview, scaled by sceneScale so a drag
// gesture shifts a large model by a proportionally larger world-space distance (and a tiny model
// by a proportionally smaller one).
void PreviewInteractionState::pan(const glm::vec2& delta) {
    panOffset += glm::vec3(-delta.x, delta.y, 0.0F) * sceneScale;
}

void PreviewInteractionState::zoom(float wheelDelta) {
    cameraDistance = std::clamp(cameraDistance * std::exp(-wheelDelta * 0.15F), 1.25F * sceneScale, 12.0F * sceneScale);
}

void PreviewInteractionState::reset() {
    orbitAngles = glm::vec2(0.0F);
    panOffset = glm::vec3(0.0F);
    cameraDistance = 4.0F * sceneScale;
}

void PreviewInteractionState::setSceneScale(float scale) {
    // Guard against degenerate models (e.g. a single point, or a load that produced no vertices)
    // collapsing the camera distance to zero, which would make the preview unusable.
    sceneScale = scale > 0.0001F ? scale : 1.0F;
    reset();
}
}  // namespace shadereditor
