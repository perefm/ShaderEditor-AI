#include "catch2/catch_test_macros.hpp"

#include "app/workspace/PreviewInteractionState.h"

TEST_CASE("preview interaction state clamps orbit and accumulates pan") {
    shadereditor::PreviewInteractionState state;
    state.orbit({0.5F, 10.0F});
    state.pan({1.0F, 2.0F});

    REQUIRE(state.orbitAngles.y <= 1.4F);
    REQUIRE(state.panOffset.x == 1.0F);
    REQUIRE(state.panOffset.y == -2.0F);
}
