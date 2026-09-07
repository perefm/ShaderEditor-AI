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
                // Fox.glb packs its diffuse texture directly in the .glb binary (glTF embedded
                // image), so it must be exposed via embeddedImageData rather than a file path.
                REQUIRE(!slot.embeddedImageData.empty());
                REQUIRE(slot.sourcePath.empty());
            }
        }
    }
    REQUIRE(foundDiffuseTexture);

    // Fox's glTF mesh must carry real UVs; a zeroed TEXCOORD_0 stream would make a correctly
    // loaded image appear as a single sampled texel across the whole model.
    bool foundNonZeroUv = false;
    for (const auto& mesh : result.document.meshes) {
        for (const auto& vertex : mesh.vertices) {
            if (vertex.texCoords.x != 0.0F || vertex.texCoords.y != 0.0F) {
                foundNonZeroUv = true;
                break;
            }
        }
        if (foundNonZeroUv) {
            break;
        }
    }
    REQUIRE(foundNonZeroUv);

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

TEST_CASE("AssimpModelLoader imports the bundled normal-map bump sample") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(modelPath("NormalTangentTest/NormalTangentTest.glb"));

    REQUIRE(result.success);
    REQUIRE(!result.document.meshes.empty());

    bool foundNormalMap = false;
    bool foundGeneratedTangent = false;
    for (const auto& mesh : result.document.meshes) {
        for (const auto& slot : mesh.material.textureSlots) {
            if (slot.shaderUniformName == "texture_normal1") {
                foundNormalMap = true;
                REQUIRE(!slot.embeddedImageData.empty());
            }
        }
        for (const auto& vertex : mesh.vertices) {
            if (glm::length(vertex.tangent) > 0.001F && glm::length(vertex.biTangent) > 0.001F) {
                foundGeneratedTangent = true;
                break;
            }
        }
    }
    REQUIRE(foundNormalMap);
    REQUIRE(foundGeneratedTangent);
}

TEST_CASE("AssimpModelLoader reports a readable error for a missing file") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(modelPath("DoesNotExist/missing.glb"));

    REQUIRE(!result.success);
    REQUIRE(!result.errorMessage.empty());
    REQUIRE(result.document.meshes.empty());
}
