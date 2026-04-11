#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace shadereditor {
struct PreviewInteractionState {
    glm::vec2 orbitAngles {0.0F, 0.0F};
    glm::vec3 panOffset {0.0F, 0.0F, 0.0F};

    void orbit(const glm::vec2& delta);
    void pan(const glm::vec2& delta);
    void reset();
};
}  // namespace shadereditor
