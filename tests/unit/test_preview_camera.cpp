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
