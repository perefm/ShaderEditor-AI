#include "rendering/models/ModelCameraResolver.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/vec4.hpp>

#include <cmath>

namespace shadereditor {
namespace {
bool isFinite(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

// Assimp reports aiCamera::mHorizontalFOV as the *full horizontal* angle in radians (see
// glTF2Importer, which computes it as 2*atan(tan(yfov/2)*aspect)). glm::perspective wants the full
// vertical angle, so invert exactly that relation using the aspect Assimp itself used.
float verticalFovFromHorizontal(float horizontalRadians, float aspectRatio, float fallbackVerticalRadians) {
    if (!std::isfinite(horizontalRadians) || horizontalRadians <= 0.0F || aspectRatio <= 0.0F) {
        return fallbackVerticalRadians;
    }
    const float verticalRadians = 2.0F * std::atan(std::tan(horizontalRadians * 0.5F) / aspectRatio);
    if (!std::isfinite(verticalRadians) || verticalRadians <= 0.0F) {
        return fallbackVerticalRadians;
    }
    return verticalRadians;
}
}

std::optional<ResolvedCamera> ModelCameraResolver::resolve(const ModelDocument& document,
                                                           int cameraIndex,
                                                           float elapsedSeconds,
                                                           int animationIndex,
                                                           AnimationLoopMode loopMode,
                                                           float viewportAspectRatio,
                                                           float defaultVerticalFovRadians,
                                                           float nearPlane,
                                                           float farPlane) const {
    if (cameraIndex < 0 || static_cast<std::size_t>(cameraIndex) >= document.cameras.size()) {
        return std::nullopt;
    }

    const ModelCamera& camera = document.cameras[static_cast<std::size_t>(cameraIndex)];

    // The camera node's world transform carries both its static placement in the scene graph and
    // any keyframe animation targeting it, so static and animated cameras share one code path.
    const glm::mat4 nodeTransform = animator_.nodeWorldTransform(document, camera.nodeName, elapsedSeconds, animationIndex, loopMode);

    const glm::vec3 worldPosition = glm::vec3(nodeTransform * glm::vec4(camera.position, 1.0F));
    // Direction/up are directions, not points, so they are transformed with w = 0 to ignore the
    // node's translation.
    glm::vec3 worldForward = glm::vec3(nodeTransform * glm::vec4(camera.lookAt, 0.0F));
    glm::vec3 worldUp = glm::vec3(nodeTransform * glm::vec4(camera.up, 0.0F));

    if (!isFinite(worldPosition) || !isFinite(worldForward) || !isFinite(worldUp)) {
        return std::nullopt;
    }
    if (glm::length(worldForward) <= 0.0F) {
        worldForward = glm::vec3(0.0F, 0.0F, -1.0F);
    }
    if (glm::length(worldUp) <= 0.0F) {
        worldUp = glm::vec3(0.0F, 1.0F, 0.0F);
    }
    worldForward = glm::normalize(worldForward);
    worldUp = glm::normalize(worldUp);

    // A forward vector parallel to up would make lookAt degenerate; nudge up to a safe axis.
    if (std::abs(glm::dot(worldForward, worldUp)) > 0.9999F) {
        worldUp = std::abs(worldForward.y) > 0.9F ? glm::vec3(0.0F, 0.0F, 1.0F) : glm::vec3(0.0F, 1.0F, 0.0F);
    }

    const float safeViewportAspect = viewportAspectRatio > 0.0F ? viewportAspectRatio : 1.0F;
    // The FOV must be un-converted with the *same* aspect Assimp used when it produced
    // mHorizontalFOV. Assimp substitutes 1.0 when the source camera declares no aspect ratio, so
    // using the viewport aspect here instead would systematically distort the vertical FOV.
    const float fovAspect = camera.aspectRatio > 0.0F ? camera.aspectRatio : 1.0F;
    const float verticalFov = verticalFovFromHorizontal(camera.horizontalFovRadians, fovAspect, defaultVerticalFovRadians);

    const float safeNear = camera.nearPlane > 0.0F ? camera.nearPlane : nearPlane;
    const float safeFar = camera.farPlane > safeNear ? camera.farPlane : farPlane;

    ResolvedCamera resolved;
    resolved.position = worldPosition;
    // lookAt(position, position + forward, up): Phoenix documented that the naive
    // lookAt(position, forward, up) form yields a mirrored/backwards view for model cameras.
    resolved.view = glm::lookAt(worldPosition, worldPosition + worldForward, worldUp);
    resolved.projection = glm::perspective(verticalFov, safeViewportAspect, safeNear, safeFar > safeNear ? safeFar : safeNear + 1.0F);
    return resolved;
}
}  // namespace shadereditor
