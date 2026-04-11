#include "app/workspace/PreviewInteractionState.h"

#include <algorithm>

namespace shadereditor {
void PreviewInteractionState::orbit(const glm::vec2& delta) {
    orbitAngles += delta;
    orbitAngles.y = std::clamp(orbitAngles.y, -1.4F, 1.4F);
}

void PreviewInteractionState::pan(const glm::vec2& delta) { panOffset += glm::vec3(delta.x, -delta.y, 0.0F); }

void PreviewInteractionState::reset() {
    orbitAngles = glm::vec2(0.0F);
    panOffset = glm::vec3(0.0F);
}
}  // namespace shadereditor
