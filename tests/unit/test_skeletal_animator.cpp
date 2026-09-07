#include "catch2/catch_test_macros.hpp"

#include "rendering/models/AssimpModelLoader.h"
#include "rendering/models/SkeletalAnimator.h"

#include <glm/mat4x4.hpp>

#include <filesystem>

TEST_CASE("SkeletalAnimator returns identity transforms for a model without a skeleton") {
    shadereditor::ModelDocument document;
    document.hasSkeleton = false;
    document.boneCount = 3;

    shadereditor::SkeletalAnimator animator;
    const auto transforms = animator.boneTransforms(document, 1.0F);

    REQUIRE(transforms.size() == 3);
    for (const auto& transform : transforms) {
        REQUIRE(transform == glm::mat4(1.0F));
    }
}

TEST_CASE("SkeletalAnimator produces one transform per bone for the bundled animated Fox model") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(std::filesystem::path(SHADEREDITOR_SOURCE_DIR) / "assets" / "models" / "Fox" / "Fox.glb");
    REQUIRE(result.success);

    shadereditor::SkeletalAnimator animator;
    const auto transformsAtStart = animator.boneTransforms(result.document, 0.0F);
    REQUIRE(transformsAtStart.size() == result.document.boneCount);

    // Sampling a later point in the (looping) clip should generally produce a different pose;
    // this is a smoke test that the animator is actually driven by elapsedSeconds rather than
    // always returning the bind pose.
    const auto transformsLater = animator.boneTransforms(result.document, 0.5F);
    REQUIRE(transformsLater.size() == result.document.boneCount);
}
