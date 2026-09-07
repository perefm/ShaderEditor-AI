#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace shadereditor {
// Stores the camera gestures currently applied to the preview scene.
struct PreviewInteractionState {
    glm::vec2 orbitAngles {0.0F, 0.0F};
    glm::vec3 panOffset {0.0F, 0.0F, 0.0F};
    float cameraDistance {4.0F};

    // Left-drag rotates the preview.
    void orbit(const glm::vec2& delta);
    // Right-drag shifts the framing without changing orbit.
    void pan(const glm::vec2& delta);
    // Mouse-wheel zoom changes the camera distance while preserving the target.
    void zoom(float wheelDelta);
    // Restores the default preview framing.
    void reset();
};
}  // namespace shadereditor
