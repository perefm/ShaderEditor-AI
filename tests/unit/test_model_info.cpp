#include "catch2/catch_test_macros.hpp"

#include "rendering/models/AssimpModelLoader.h"
#include "rendering/models/ModelCameraResolver.h"
#include "rendering/models/ModelInfoSummary.h"
#include "rendering/models/SkeletalAnimator.h"

#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <filesystem>

namespace {
std::filesystem::path samplePath(const char* relative) {
    return std::filesystem::path(SHADEREDITOR_SOURCE_DIR) / "assets" / "models" / "KeyframeSamples" / relative;
}
}

TEST_CASE("Keyframe object sample exposes animated nodes and no cameras") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(samplePath("KeyframeCube.gltf"));

    REQUIRE(result.success);
    REQUIRE(result.document.animations.size() == 2);
    REQUIRE(result.document.cameras.empty());

    // Both cubes must survive import as separate draw instances. The sample shares one mesh
    // between two nodes (which is also what aiProcess_FindInstances produces from identical
    // meshes), so drawing one entry per unique mesh would silently drop one of the two cubes.
    REQUIRE(result.document.meshes.size() == 1);
    REQUIRE(result.document.meshInstances.size() == 2);
    REQUIRE(result.document.meshInstances[0].nodeIndex != result.document.meshInstances[1].nodeIndex);

    // Each mesh must know which scene node places it: node-level keyframe animation lives in that
    // node's transform, so a mesh with no node binding would render frozen at the origin.
    for (const auto& mesh : result.document.meshes) {
        REQUIRE(!mesh.nodeName.empty());
    }

    // Every clip must carry at least one channel, otherwise the sample would render statically.
    for (const auto& clip : result.document.animations) {
        REQUIRE(!clip.channels.empty());
        REQUIRE(clip.durationSeconds > 0.0);
    }
}

TEST_CASE("Node keyframes actually move the mesh's node over time") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(samplePath("KeyframeCube.gltf"));
    REQUIRE(result.success);

    shadereditor::SkeletalAnimator animator;
    const auto at = [&](float seconds) {
        return animator.nodeWorldTransform(result.document, "OrbitingCube", seconds, 0,
                                           shadereditor::AnimationLoopMode::Loop);
    };
    const glm::mat4 start = at(0.0F);
    const glm::mat4 later = at(1.0F);

    // Translation lives in the 4th column; the orbiting cube must actually travel.
    REQUIRE(glm::distance(glm::vec3(start[3]), glm::vec3(later[3])) > 0.5F);
}

TEST_CASE("Keyframe camera sample exposes an animated camera that Model info reports") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(samplePath("KeyframeCamera.gltf"));

    REQUIRE(result.success);
    REQUIRE(result.document.cameras.size() == 1);
    REQUIRE(result.document.animations.size() == 1);

    const auto summary = shadereditor::buildModelInfoSummary(result.document);
    REQUIRE(summary.hasModel);
    REQUIRE(summary.meshCount == result.document.meshes.size());
    REQUIRE(summary.triangleCount == summary.indexCount / 3);
    REQUIRE(summary.materialCount == result.document.materialCount);
    REQUIRE(summary.cameras.size() == 1);
    REQUIRE(summary.cameras.front().animated);
}

TEST_CASE("Model camera reconstructs the authored field of view") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(samplePath("KeyframeCamera.gltf"));
    REQUIRE(result.success);
    REQUIRE(result.document.cameras.size() == 1);

    shadereditor::ModelCameraResolver resolver;
    const auto resolved = resolver.resolve(result.document, 0, 0.0F, 0, shadereditor::AnimationLoopMode::Loop,
                                           16.0F / 9.0F, 0.785F, 0.1F, 100.0F);
    REQUIRE(resolved.has_value());

    // The sample authors yfov = 0.7854 rad (45 deg). Assimp stores the *full horizontal* FOV, so
    // treating it as a half-angle would double the reconstructed FOV to ~90 deg and make the whole
    // scene look far away. Recover the vertical FOV from the projection matrix: P[1][1] = 1/tan(fov/2).
    const float verticalFov = 2.0F * std::atan(1.0F / resolved->projection[1][1]);
    REQUIRE(std::abs(verticalFov - 0.7854F) < 0.01F);

    // The projection must use the *viewport* aspect so the preview is never stretched.
    const float aspect = resolved->projection[1][1] / resolved->projection[0][0];
    REQUIRE(std::abs(aspect - 16.0F / 9.0F) < 0.01F);
}

TEST_CASE("ModelCameraResolver moves an animated camera over time") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(samplePath("KeyframeCamera.gltf"));
    REQUIRE(result.success);

    shadereditor::ModelCameraResolver resolver;
    const auto atStart = resolver.resolve(result.document, 0, 0.0, 0,
                                          shadereditor::AnimationLoopMode::Loop, 16.0F / 9.0F, 0.785F, 0.1F, 100.0F);
    const auto atQuarter = resolver.resolve(result.document, 0, 1.5, 0,
                                            shadereditor::AnimationLoopMode::Loop, 16.0F / 9.0F, 0.785F, 0.1F, 100.0F);

    REQUIRE(atStart.has_value());
    REQUIRE(atQuarter.has_value());
    // A keyframed camera must yield a different world position (and therefore a different
    // uCameraPos) as playback advances.
    REQUIRE(glm::distance(atStart->position, atQuarter->position) > 0.5F);

    // An out-of-range index must fall back to "no scene camera" so the free camera stays in use.
    REQUIRE(!resolver.resolve(result.document, 7, 0.0, 0, shadereditor::AnimationLoopMode::Loop,
                              16.0F / 9.0F, 0.785F, 0.1F, 100.0F)
                 .has_value());
}

TEST_CASE("Animated camera keeps looking at the scene origin") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(samplePath("KeyframeCamera.gltf"));
    REQUIRE(result.success);

    shadereditor::ModelCameraResolver resolver;
    // The sample orbits the camera around the origin while yawing to keep the cube centred, so at
    // every point of the orbit the target must land in front of the camera, near the view centre.
    for (float seconds : {0.0F, 1.5F, 3.0F, 4.5F}) {
        const auto resolved = resolver.resolve(result.document, 0, seconds, 0,
                                               shadereditor::AnimationLoopMode::Loop, 16.0F / 9.0F, 0.785F, 0.1F, 100.0F);
        REQUIRE(resolved.has_value());

        const glm::vec4 originInView = resolved->view * glm::vec4(0.0F, 0.0F, 0.0F, 1.0F);
        // OpenGL view space looks down -Z, so a target in front has negative z. A mirrored or
        // wrongly-oriented view (the classic lookAt(pos, dir, up) mistake) would flip this sign.
        REQUIRE(originInView.z < 0.0F);
        // And it must be centred, not off to one side: x/y stay small next to the view depth.
        REQUIRE(std::abs(originInView.x) < 0.5F);
        REQUIRE(std::abs(originInView.y) < 2.0F);
    }
}

TEST_CASE("buildModelInfoSummary reports an empty document as having no model") {
    const shadereditor::ModelDocument empty;
    const auto summary = shadereditor::buildModelInfoSummary(empty);
    REQUIRE(!summary.hasModel);
    REQUIRE(summary.meshCount == 0);
    REQUIRE(summary.cameras.empty());
}


TEST_CASE("Batched draw path resolves mesh node indices and deduplicates materials") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(samplePath("KeyframeCube.gltf"));
    REQUIRE(result.success);

    // The per-frame draw loop indexes the precomputed transform array by nodeIndex instead of
    // walking the hierarchy per mesh, so every mesh must resolve to a valid node slot.
    REQUIRE(!result.document.meshes.empty());
    for (const auto& mesh : result.document.meshes) {
        REQUIRE(mesh.nodeIndex >= 0);
        REQUIRE(static_cast<std::size_t>(mesh.nodeIndex) < result.document.sceneNodes.size());
        REQUIRE(result.document.sceneNodes[static_cast<std::size_t>(mesh.nodeIndex)].name == mesh.nodeName);
        REQUIRE(mesh.materialIndex >= 0);
        REQUIRE(static_cast<std::size_t>(mesh.materialIndex) < result.document.materials.size());
    }
    REQUIRE(result.document.materials.size() <= result.document.meshes.size());
}

TEST_CASE("Batched node transforms match the per-mesh hierarchy walk") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(samplePath("KeyframeCube.gltf"));
    REQUIRE(result.success);

    shadereditor::SkeletalAnimator animator;
    const float elapsed = 0.75F;
    const auto batched =
        animator.nodeWorldTransforms(result.document, elapsed, 0, shadereditor::AnimationLoopMode::Loop);
    REQUIRE(batched.size() == result.document.sceneNodes.size());

    for (const auto& mesh : result.document.meshes) {
        const auto reference =
            animator.nodeWorldTransform(result.document, mesh.nodeName, elapsed, 0, shadereditor::AnimationLoopMode::Loop);
        const auto& fast = batched[static_cast<std::size_t>(mesh.nodeIndex)];
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                REQUIRE(std::abs(reference[col][row] - fast[col][row]) < 1e-5F);
            }
        }
    }
}


TEST_CASE("Every scene-node mesh reference becomes a draw instance") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(samplePath("KeyframeCube.gltf"));
    REQUIRE(result.success);

    // Meshes referenced by several nodes (instancing) must be drawn once per placement. Drawing
    // one entry per unique mesh silently dropped every copy beyond the first.
    REQUIRE(!result.document.meshInstances.empty());
    REQUIRE(result.document.meshInstances.size() >= result.document.meshes.size());
    for (const auto& instance : result.document.meshInstances) {
        REQUIRE(instance.meshIndex < result.document.meshes.size());
        REQUIRE(instance.nodeIndex >= 0);
        REQUIRE(static_cast<std::size_t>(instance.nodeIndex) < result.document.sceneNodes.size());
    }

    // Every mesh must be placed at least once, otherwise it would never be drawn.
    std::vector<bool> drawn(result.document.meshes.size(), false);
    for (const auto& instance : result.document.meshInstances) {
        drawn[instance.meshIndex] = true;
    }
    for (const bool meshDrawn : drawn) {
        REQUIRE(meshDrawn);
    }

    // The two instances share a mesh, so they are only distinguishable by their node placement.
    // Identical transforms would draw both cubes on top of each other.
    shadereditor::SkeletalAnimator animator;
    const auto transforms =
        animator.nodeWorldTransforms(result.document, 0.0F, 0, shadereditor::AnimationLoopMode::Loop);
    const glm::mat4 first = transforms[static_cast<std::size_t>(result.document.meshInstances[0].nodeIndex)];
    const glm::mat4 second = transforms[static_cast<std::size_t>(result.document.meshInstances[1].nodeIndex)];
    REQUIRE(glm::length(glm::vec3(first[3]) - glm::vec3(second[3])) > 1e-3F);
}

TEST_CASE("Bounds account for instance placement rather than mesh-local positions") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(samplePath("KeyframeCube.gltf"));
    REQUIRE(result.success);

    // Scenes built from repeated props keep every copy at the origin in mesh-local space, so
    // ignoring node placement would measure the model far smaller than it is drawn.
    std::vector<glm::mat4> world(result.document.sceneNodes.size(), glm::mat4(1.0F));
    for (std::size_t i = 0; i < result.document.sceneNodes.size(); ++i) {
        const auto& node = result.document.sceneNodes[i];
        world[i] = node.parentIndex >= 0 ? world[static_cast<std::size_t>(node.parentIndex)] * node.localTransform
                                         : node.localTransform;
    }

    glm::vec3 expectedMin(0.0F);
    glm::vec3 expectedMax(0.0F);
    bool seen = false;
    for (const auto& instance : result.document.meshInstances) {
        const glm::mat4 placement =
            instance.nodeIndex >= 0 ? world[static_cast<std::size_t>(instance.nodeIndex)] : glm::mat4(1.0F);
        for (const auto& vertex : result.document.meshes[instance.meshIndex].vertices) {
            const glm::vec3 position = glm::vec3(placement * glm::vec4(vertex.position, 1.0F));
            if (!seen) {
                expectedMin = position;
                expectedMax = position;
                seen = true;
            } else {
                expectedMin = glm::min(expectedMin, position);
                expectedMax = glm::max(expectedMax, position);
            }
        }
    }
    REQUIRE(seen);
    for (int i = 0; i < 3; ++i) {
        REQUIRE(std::abs(result.document.boundsMin[i] - expectedMin[i]) < 1e-4F);
        REQUIRE(std::abs(result.document.boundsMax[i] - expectedMax[i]) < 1e-4F);
    }
}
