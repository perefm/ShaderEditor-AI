#include "catch2/catch_test_macros.hpp"

#include "rendering/models/AssimpModelLoader.h"
#include "rendering/models/SkeletalAnimator.h"

#include <glm/mat4x4.hpp>

#include <cmath>
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

TEST_CASE("SkeletalAnimator falls back to the bind pose when no animation is selected") {
    shadereditor::AssimpModelLoader loader;
    const auto result = loader.load(std::filesystem::path(SHADEREDITOR_SOURCE_DIR) / "assets" / "models" / "Fox" / "Fox.glb");
    REQUIRE(result.success);
    REQUIRE(!result.document.animations.empty());

    shadereditor::SkeletalAnimator animator;
    // A negative animationIndex (the RenderSession default for "no animation selected") must
    // produce identity bone transforms regardless of elapsed time, unlike the animated case.
    const auto bindPoseA = animator.boneTransforms(result.document, 0.0F, -1);
    const auto bindPoseB = animator.boneTransforms(result.document, 5.0F, -1);
    REQUIRE(bindPoseA.size() == result.document.boneCount);
    REQUIRE(bindPoseA == bindPoseB);

    // An out-of-range positive index should behave the same way (defensive fallback), not crash
    // or silently clamp to a valid clip.
    const auto outOfRangePose = animator.boneTransforms(result.document, 0.0F, static_cast<int>(result.document.animations.size()) + 5);
    REQUIRE(outOfRangePose == bindPoseA);
}

TEST_CASE("normalizeAnimationTime loops or holds according to the selected policy") {
    using shadereditor::AnimationLoopMode;
    using shadereditor::normalizeAnimationTime;

    // Looping wraps past the clip duration; holding clamps to the final keyframe (FR-008).
    REQUIRE(std::abs(normalizeAnimationTime(2.5, 2.0, AnimationLoopMode::Loop) - 0.5) < 1e-9);
    REQUIRE(std::abs(normalizeAnimationTime(2.5, 2.0, AnimationLoopMode::Hold) - 2.0) < 1e-9);
    REQUIRE(std::abs(normalizeAnimationTime(1.25, 2.0, AnimationLoopMode::Loop) - 1.25) < 1e-9);

    // Degenerate clips and negative times must never produce NaN or negative sample positions.
    REQUIRE(std::abs(normalizeAnimationTime(3.0, 0.0, AnimationLoopMode::Loop)) < 1e-9);
    REQUIRE(normalizeAnimationTime(-1.5, 2.0, AnimationLoopMode::Loop) >= 0.0);
    REQUIRE(normalizeAnimationTime(-1.5, 2.0, AnimationLoopMode::Hold) >= 0.0);
}
