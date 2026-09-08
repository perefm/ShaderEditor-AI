#pragma once

#include "rendering/models/ModelDocument.h"

#include <glm/vec3.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace shadereditor {
// Read-only snapshot of everything the "Model info" panel displays about the currently loaded
// model. Built once per model load from the already-imported ModelDocument (never from a second
// Assimp parse), so the panel can only ever report what was actually imported.
struct ModelInfoSummary {
    bool hasModel {false};
    std::string name;
    std::string sourcePath;

    std::size_t meshCount {0};
    std::size_t vertexCount {0};
    std::size_t triangleCount {0};
    std::size_t indexCount {0};
    std::size_t materialCount {0};

    bool hasTextures {false};
    std::size_t textureCount {0};
    std::size_t embeddedTextureCount {0};
    // Texture uniform type ("diffuse", "normal", ...) paired with how many slots use it.
    std::vector<std::pair<std::string, std::size_t>> texturesByType;

    bool hasSkeleton {false};
    std::size_t boneCount {0};
    std::size_t sceneNodeCount {0};

    struct AnimationInfo {
        std::string name;
        float durationSeconds {0.0F};
        std::size_t channelCount {0};
    };
    std::vector<AnimationInfo> animations;

    struct CameraInfo {
        std::string name;
        bool animated {false};
    };
    std::vector<CameraInfo> cameras;

    glm::vec3 boundsMin {0.0F};
    glm::vec3 boundsMax {0.0F};
    glm::vec3 boundsSize {0.0F};
    float boundingRadius {0.0F};
};

// Pure ModelDocument -> ModelInfoSummary projection. A default-constructed (hasModel == false)
// summary is what the panel renders as its "no model loaded" empty state.
[[nodiscard]] ModelInfoSummary buildModelInfoSummary(const ModelDocument& document);
}  // namespace shadereditor
