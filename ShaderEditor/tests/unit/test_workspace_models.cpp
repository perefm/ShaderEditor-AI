#include "catch2/catch_test_macros.hpp"

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
