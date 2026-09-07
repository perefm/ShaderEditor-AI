#include "catch2/catch_test_macros.hpp"

#include "rendering/models/AssimpModelLoader.h"

#include <filesystem>

namespace {
std::filesystem::path modelPath(const char* relative) {
    return std::filesystem::path(SHADEREDITOR_SOURCE_DIR) / "assets" / "models" / relative;
}
}

TEST_CASE("AssimpModelLoader imports the bundled animated Fox model") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(modelPath("Fox/Fox.glb"));

    REQUIRE(result.success);
    REQUIRE(result.errorMessage.empty());
    REQUIRE(!result.document.meshes.empty());
    REQUIRE(result.document.hasSkeleton);
    REQUIRE(result.document.boneCount > 0);
    REQUIRE(!result.document.animations.empty());
    REQUIRE(!result.document.sceneNodes.empty());

    // At least one mesh must carry a diffuse texture slot named per Phoenix's convention
    // (FR-015), since the bundled Fox asset is diffuse-textured.
    bool foundDiffuseTexture = false;
    for (const auto& mesh : result.document.meshes) {
        for (const auto& slot : mesh.material.textureSlots) {
            if (slot.shaderUniformName == "texture_diffuse1") {
                foundDiffuseTexture = true;
            }
        }
    }
    REQUIRE(foundDiffuseTexture);

    // Every vertex's bone weights must be within the fixed-size Phoenix-compatible layout.
    for (const auto& mesh : result.document.meshes) {
        for (const auto& vertex : mesh.vertices) {
            REQUIRE(vertex.boneIds.size() == shadereditor::kMaxBonesPerVertex);
            REQUIRE(vertex.boneWeights.size() == shadereditor::kMaxBonesPerVertex);
        }
    }

    // The computed AABB must be non-degenerate and actually enclose the model's geometry, since
    // it drives the preview camera's proportional zoom/orbit/pan scaling.
    REQUIRE(result.document.boundsMax.x > result.document.boundsMin.x);
    REQUIRE(result.document.boundingRadius() > 0.0F);
}

TEST_CASE("AssimpModelLoader imports the bundled animated CesiumMan (PBR) model") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(modelPath("CesiumMan/CesiumMan.glb"));

    REQUIRE(result.success);
    REQUIRE(!result.document.meshes.empty());
    REQUIRE(result.document.hasSkeleton);
    REQUIRE(result.document.boneCount > 0);
    REQUIRE(!result.document.animations.empty());
}

TEST_CASE("AssimpModelLoader reports a readable error for a missing file") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(modelPath("DoesNotExist/missing.glb"));

    REQUIRE(!result.success);
    REQUIRE(!result.errorMessage.empty());
    REQUIRE(result.document.meshes.empty());
}
