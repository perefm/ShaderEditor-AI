#include "rendering/opengl/PreviewCamera.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

namespace shadereditor {
namespace {
// Orbit moves the free camera around the target; it must never touch the model matrix, because
// objects have to stay at the transform authored in the file. Baking orbit into the model would
// also leak the free-camera framing into scene cameras, which share those object transforms.
glm::mat4 orbitRotation(const PreviewInteractionState& interactionState) {
    glm::mat4 rotation = glm::mat4(1.0F);
    rotation = glm::rotate(rotation, interactionState.orbitAngles.x, glm::vec3(0.0F, 1.0F, 0.0F));
    rotation = glm::rotate(rotation, interactionState.orbitAngles.y, glm::vec3(1.0F, 0.0F, 0.0F));
    // A pure rotation's inverse is its transpose; orbiting the camera by the inverse keeps the drag
    // direction users already expect from when the rotation was applied to the scene instead.
    return glm::transpose(rotation);
}
}  // namespace

glm::vec3 PreviewCamera::position(const PreviewInteractionState& interactionState) const {
    const glm::vec3 offset =
        glm::vec3(orbitRotation(interactionState) * glm::vec4(0.0F, 0.0F, interactionState.cameraDistance, 0.0F));
    return interactionState.panOffset + offset;
}

glm::mat4 PreviewCamera::modelMatrix(const PreviewInteractionState& interactionState) const {
    // Free-camera interaction never modifies object placement.
    (void)interactionState;
    return glm::mat4(1.0F);
}

glm::mat4 PreviewCamera::view(const PreviewInteractionState& interactionState) const {
    // Orbit swings the camera around the preview target; pan slides that target across the scene.
    const glm::vec3 target = interactionState.panOffset;
    const glm::vec3 eye = position(interactionState);
    const glm::vec3 up = glm::vec3(orbitRotation(interactionState) * glm::vec4(0.0F, 1.0F, 0.0F, 0.0F));
    return glm::lookAt(eye, target, up);
}

glm::mat4 PreviewCamera::projection(const PreviewInteractionState& interactionState, float aspectRatio) const {
    const float safeAspectRatio = aspectRatio > 0.0F ? aspectRatio : 1.0F;
    // Near/far clip planes scale with sceneScale so imported models much larger or smaller than
    // the ~1-unit built-in primitives are neither clipped by a too-near far plane nor z-fighting
    // against a near plane that is too far away relative to their size.
    const float nearPlane = 0.05F * interactionState.sceneScale;
    const float farPlane = 200.0F * interactionState.sceneScale;
    return glm::perspective(glm::radians(45.0F), safeAspectRatio, nearPlane, farPlane);
}

glm::mat4 PreviewCamera::viewProjection(const PreviewInteractionState& interactionState, float aspectRatio) const {
    return projection(interactionState, aspectRatio) * view(interactionState) * modelMatrix(interactionState);
}
}  // namespace shadereditor
