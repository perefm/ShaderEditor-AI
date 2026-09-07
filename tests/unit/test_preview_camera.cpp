#include "catch2/catch_test_macros.hpp"

#include "app/workspace/PreviewInteractionState.h"
#include "rendering/opengl/PreviewCamera.h"

TEST_CASE("preview interaction state clamps orbit and accumulates pan") {
    shadereditor::PreviewInteractionState state;
    state.orbit({0.5F, 10.0F});
    state.pan({1.0F, 2.0F});

    REQUIRE(state.orbitAngles.y <= 1.4F);
    REQUIRE(state.panOffset.x == -1.0F);
    REQUIRE(state.panOffset.y == 2.0F);
}

TEST_CASE("preview camera position stays stable while the preview model rotates") {
    shadereditor::PreviewInteractionState state;
    shadereditor::PreviewCamera camera;

    const glm::vec3 initialPosition = camera.position(state);
    state.orbit({1.0F, 0.5F});

    REQUIRE(camera.position(state) == initialPosition);
}

TEST_CASE("setSceneScale rescales default distance, zoom clamp and pan sensitivity") {
    shadereditor::PreviewInteractionState state;

    // A large imported model (bounding radius 50) should get a proportionally larger default
    // camera distance and zoom range than the ~1-unit built-in primitives.
    state.setSceneScale(50.0F);
    REQUIRE(state.cameraDistance == 4.0F * 50.0F);
    REQUIRE(state.orbitAngles == glm::vec2(0.0F));
    REQUIRE(state.panOffset == glm::vec3(0.0F));

    // Zooming all the way out should not exceed the scaled clamp.
    state.zoom(-1000.0F);
    REQUIRE(state.cameraDistance <= 12.0F * 50.0F);
    // Zooming all the way in should not go below the scaled clamp.
    state.zoom(1000.0F);
    REQUIRE(state.cameraDistance >= 1.25F * 50.0F);

    // Pan sensitivity scales with sceneScale so a fixed drag delta moves a large model further.
    state.panOffset = glm::vec3(0.0F);
    state.pan({1.0F, 0.0F});
    REQUIRE(state.panOffset.x == -1.0F * 50.0F);

    // Switching back to a primitive (sceneScale 1.0) restores the original defaults.
    state.setSceneScale(1.0F);
    REQUIRE(state.cameraDistance == 4.0F);

    // A non-positive/degenerate scale (e.g. a model that failed to produce a bounding radius)
    // should not leave the camera distance at zero.
    state.setSceneScale(0.0F);
    REQUIRE(state.sceneScale == 1.0F);
    REQUIRE(state.cameraDistance == 4.0F);
}
