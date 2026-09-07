#include "catch2/catch_test_macros.hpp"

#include "app/workspace/UniformState.h"
#include "editor/ShaderPairDocument.h"
#include "rendering/shaders/UniformDefinition.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

TEST_CASE("shader document dirty state") {
    shadereditor::ShaderPairDocument document;
    document.markDirty();
    REQUIRE(document.isDirty);
    document.markSaved();
    REQUIRE(!document.isDirty);
}

TEST_CASE("uniform definition stores applied values") {
    shadereditor::UniformDefinition uniform;
    uniform.name = "u_color";
    uniform.currentValue = 1.0F;
    REQUIRE(std::get<float>(uniform.currentValue) == 1.0F);
}

TEST_CASE("uniform definition supports glm vector and matrix values") {
    shadereditor::UniformDefinition vecUniform;
    vecUniform.currentValue = glm::vec2(1.0F, 2.0F);
    REQUIRE(std::get<glm::vec2>(vecUniform.currentValue).x == 1.0F);

    shadereditor::UniformDefinition matUniform;
    matUniform.currentValue = glm::mat4(1.0F);
    REQUIRE(std::get<glm::mat4>(matUniform.currentValue)[0][0] == 1.0F);
}

TEST_CASE("uniform state preserves compatible values across refresh") {
    shadereditor::UniformState state;
    shadereditor::UniformDefinition original;
    original.name = "u_value";
    original.kind = "float";
    original.defaultValue = 0.0F;
    original.currentValue = 2.5F;
    state.setDefinitions({original});
    state.apply("u_value", 4.0F);

    shadereditor::UniformDefinition refreshed = original;
    refreshed.currentValue = refreshed.defaultValue;
    state.setDefinitions({refreshed});

    REQUIRE(std::get<float>(state.definitions().front().currentValue) == 4.0F);
}

TEST_CASE("uniform state resets changed types and removes stale definitions") {
    shadereditor::UniformState state;
    shadereditor::UniformDefinition original;
    original.name = "u_value";
    original.kind = "float";
    original.defaultValue = 0.0F;
    original.currentValue = 4.0F;
    state.setDefinitions({original});

    shadereditor::UniformDefinition changed = original;
    changed.kind = "vec2";
    changed.defaultValue = glm::vec2(1.0F, 2.0F);
    changed.currentValue = changed.defaultValue;
    shadereditor::UniformDefinition added;
    added.name = "u_added";
    added.kind = "int";
    added.defaultValue = 3;
    added.currentValue = added.defaultValue;
    state.setDefinitions({changed, added});

    REQUIRE(state.definitions().size() == 2);
    REQUIRE(std::get<glm::vec2>(state.definitions()[0].currentValue).x == 1.0F);
    REQUIRE(std::get<int>(state.definitions()[1].currentValue) == 3);
}

TEST_CASE("uniform state can become empty after successful refresh") {
    shadereditor::UniformState state;
    shadereditor::UniformDefinition definition;
    definition.name = "u_value";
    state.setDefinitions({definition});
    state.setDefinitions({});
    REQUIRE(state.definitions().empty());
}
