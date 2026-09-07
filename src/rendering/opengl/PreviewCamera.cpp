#include "rendering/opengl/PreviewCamera.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

namespace shadereditor {
glm::vec3 PreviewCamera::position(const PreviewInteractionState& interactionState) const {
    // Orbit is applied to the preview model below, so the camera remains fixed
    // relative to the panned target.
    return interactionState.panOffset + glm::vec3(0.0F, 0.0F, interactionState.cameraDistance);
}

glm::mat4 PreviewCamera::viewProjection(const PreviewInteractionState& interactionState, float aspectRatio) const {
    const float safeAspectRatio = aspectRatio > 0.0F ? aspectRatio : 1.0F;
    glm::mat4 projection = glm::perspective(glm::radians(45.0F), safeAspectRatio, 0.1F, 100.0F);

    // Orbit rotates the scene around the preview target; pan moves the camera framing across the scene.
    const glm::vec3 target = interactionState.panOffset;
    const glm::vec3 eye = position(interactionState);
    glm::mat4 view = glm::lookAt(eye, target, glm::vec3(0.0F, 1.0F, 0.0F));

    glm::mat4 model = glm::mat4(1.0F);
    model = glm::rotate(model, interactionState.orbitAngles.x, glm::vec3(0.0F, 1.0F, 0.0F));
    model = glm::rotate(model, interactionState.orbitAngles.y, glm::vec3(1.0F, 0.0F, 0.0F));

    return projection * view * model;
}
}  // namespace shadereditor
