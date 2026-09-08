#pragma once

#include "rendering/models/ModelDocument.h"
#include "rendering/models/SkeletalAnimator.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <optional>

namespace shadereditor {
// Fully resolved camera state for one frame, whatever its source (free preview camera or a
// model-authored camera, static or keyframe-animated). Everything the engine matrix uniforms
// need comes from this single struct so "view", "projection", "MVP" and "uCameraPos" can never
// describe different cameras.
struct ResolvedCamera {
    glm::mat4 view {1.0F};
    glm::mat4 projection {1.0F};
    glm::vec3 position {0.0F};
};

// Resolves a camera authored inside a model file into view/projection matrices, applying the
// active animation clip to the camera's scene node when that node is keyframed (Phoenix does the
// same by resolving the model camera in Model::PreCalc before reading the view matrix).
class ModelCameraResolver {
  public:
    // Returns std::nullopt when `cameraIndex` is negative or out of range, which is how callers
    // fall back to the free preview camera (mirroring Phoenix's CameraNumber < 0 convention).
    // `defaultVerticalFovRadians`, `nearPlane` and `farPlane` are used when the authored camera
    // does not specify them.
    [[nodiscard]] std::optional<ResolvedCamera> resolve(const ModelDocument& document,
                                                        int cameraIndex,
                                                        float elapsedSeconds,
                                                        int animationIndex,
                                                        AnimationLoopMode loopMode,
                                                        float viewportAspectRatio,
                                                        float defaultVerticalFovRadians,
                                                        float nearPlane,
                                                        float farPlane) const;

  private:
    SkeletalAnimator animator_;
};
}  // namespace shadereditor
