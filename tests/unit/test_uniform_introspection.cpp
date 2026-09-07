#include "catch2/catch_test_macros.hpp"

#include "editor/ShaderPairDocument.h"
#include "rendering/shaders/UniformIntrospectionService.h"

TEST_CASE("uniform introspection detects vector and matrix uniforms") {
    shadereditor::ShaderPairDocument document;
    document.vertexSource = "uniform mat4 MVP; uniform vec3 uCameraPos; uniform vec2 u_offset; void main(){}";
    document.fragmentSource = "uniform vec3 u_tint; uniform mat3 u_basis; uniform mat2 u_uv; void main(){}";

    shadereditor::UniformIntrospectionService introspection;
    const auto uniforms = introspection.discover(document);

    REQUIRE(uniforms.size() == 4);
    REQUIRE(uniforms[0].kind == "vec2");
    REQUIRE(uniforms[1].kind == "vec3");
}
