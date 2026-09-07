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

TEST_CASE("uniform introspection recognizes Phoenix auto-uniforms as read-only") {
    shadereditor::ShaderPairDocument document;
    // "t"/"tend"/"beat" (playback clock), Mat_Ka/Mat_Kd/Mat_Ks/Mat_KsStrenght (per-mesh material)
    // and gBones[] (skeletal animation) are all supplied by the app/engine, never the user.
    document.fragmentSource =
        "uniform float t; uniform float tend; uniform float beat; "
        "uniform vec3 Mat_Ka; uniform vec3 Mat_Kd; uniform vec3 Mat_Ks; "
        "uniform float Mat_KsStrenght; uniform mat4 gBones[100]; "
        "uniform float u_userValue; void main(){}";

    shadereditor::UniformIntrospectionService introspection;
    const auto uniforms = introspection.discover(document);

    REQUIRE(uniforms.size() == 9);
    for (const auto& uniform : uniforms) {
        if (uniform.name == "u_userValue") {
            REQUIRE(uniform.provenance == shadereditor::UniformProvenance::User);
            REQUIRE(uniform.editable);
            continue;
        }
        // "gBones" must have its "[100]" array suffix stripped so it can be matched by name.
        REQUIRE(uniform.name.find('[') == std::string::npos);
        REQUIRE(uniform.provenance == shadereditor::UniformProvenance::PhoenixAuto);
        REQUIRE(!uniform.editable);
    }
}
