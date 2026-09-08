#include "catch2/catch_test_macros.hpp"

#include "app/workspace/PreviewInteractionState.h"
#include "rendering/opengl/PreviewCamera.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#include <cmath>

TEST_CASE("preview interaction state clamps orbit and accumulates pan") {
    shadereditor::PreviewInteractionState state;
    state.orbit({0.5F, 10.0F});
    state.pan({1.0F, 2.0F});

    REQUIRE(state.orbitAngles.y <= 1.4F);
    REQUIRE(state.panOffset.x == -1.0F);
    REQUIRE(state.panOffset.y == 2.0F);
}

TEST_CASE("orbit moves the free camera and never the model matrix") {
    shadereditor::PreviewInteractionState state;
    shadereditor::PreviewCamera camera;

    const glm::vec3 initialPosition = camera.position(state);
    state.orbit({1.0F, 0.5F});

    // Orbiting must move the eye...
    REQUIRE(camera.position(state) != initialPosition);
    REQUIRE(std::abs(glm::length(camera.position(state)) - state.cameraDistance) < 1e-4F);

    // ...and must leave object placement untouched, otherwise the rotation would still be visible
    // after switching to a scene camera.
    const glm::mat4 model = camera.modelMatrix(state);
    REQUIRE(model == glm::mat4(1.0F));
}

TEST_CASE("panning moves the camera without displacing the model matrix") {
    shadereditor::PreviewInteractionState state;
    shadereditor::PreviewCamera camera;

    state.pan({1.0F, 2.0F});

    REQUIRE(camera.position(state).x == state.panOffset.x);
    REQUIRE(camera.position(state).y == state.panOffset.y);
    REQUIRE(camera.modelMatrix(state) == glm::mat4(1.0F));
}

TEST_CASE("orbiting the camera frames the scene exactly as rotating the scene used to") {
    shadereditor::PreviewInteractionState state;
    shadereditor::PreviewCamera camera;
    state.orbit({0.7F, 0.3F});

    // Reference: the previous behaviour rotated geometry by this matrix in front of a fixed eye.
    glm::mat4 legacyModel = glm::mat4(1.0F);
    legacyModel = glm::rotate(legacyModel, state.orbitAngles.x, glm::vec3(0.0F, 1.0F, 0.0F));
    legacyModel = glm::rotate(legacyModel, state.orbitAngles.y, glm::vec3(1.0F, 0.0F, 0.0F));
    const glm::mat4 legacyView = glm::lookAt(glm::vec3(0.0F, 0.0F, state.cameraDistance), glm::vec3(0.0F),
                                             glm::vec3(0.0F, 1.0F, 0.0F));

    const glm::vec4 point(1.3F, -0.4F, 0.9F, 1.0F);
    const glm::vec4 expected = legacyView * legacyModel * point;
    const glm::vec4 actual = camera.view(state) * camera.modelMatrix(state) * point;

    for (int i = 0; i < 4; ++i) {
        REQUIRE(std::abs(expected[i] - actual[i]) < 1e-4F);
    }
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
