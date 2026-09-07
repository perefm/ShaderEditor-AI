#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace shadereditor {
// Stores the camera gestures currently applied to the preview scene.
struct PreviewInteractionState {
    glm::vec2 orbitAngles {0.0F, 0.0F};
    glm::vec3 panOffset {0.0F, 0.0F, 0.0F};
    float cameraDistance {4.0F};

    // Scale factor derived from the active model's bounding size (built-in primitives use the
    // default 1.0, since they are already ~1 unit across). Pan sensitivity and the zoom distance
    // clamp range are multiplied by this so that large/small imported models can still be
    // orbited, panned and zoomed comfortably instead of being stuck with primitive-tuned values.
    float sceneScale {1.0F};

    // Left-drag rotates the preview.
    void orbit(const glm::vec2& delta);
    // Right-drag shifts the framing without changing orbit; magnitude scales with sceneScale.
    void pan(const glm::vec2& delta);
    // Mouse-wheel zoom changes the camera distance while preserving the target; the min/max
    // distance clamp scales with sceneScale.
    void zoom(float wheelDelta);
    // Restores the default preview framing for the current sceneScale.
    void reset();
    // Sets a new sceneScale (e.g. after loading a model) and immediately reframes the camera
    // to a sensible default distance for that scale.
    void setSceneScale(float scale);
};
}  // namespace shadereditor
