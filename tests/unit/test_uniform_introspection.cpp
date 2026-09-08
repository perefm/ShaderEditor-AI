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
    // Playback, viewport, material, and skeletal-animation values are supplied by the app/engine,
    // never the user.
    document.fragmentSource =
        "uniform float t; uniform float tend; uniform float beat; "
        "uniform float vpWidth; uniform float vpHeight; uniform float aspectRatio; "
        "uniform vec3 Mat_Ka; uniform vec3 Mat_Kd; uniform vec3 Mat_Ks; "
        "uniform float Mat_KsStrenght; uniform mat4 gBones[100]; "
        "uniform float u_userValue; void main(){}";

    shadereditor::UniformIntrospectionService introspection;
    const auto uniforms = introspection.discover(document);

    REQUIRE(uniforms.size() == 12);
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

TEST_CASE("viewport Phoenix auto-uniforms require scalar float declarations") {
    shadereditor::ShaderPairDocument document;
    document.fragmentSource =
        "uniform vec2 vpWidth; "
        "uniform int vpHeight; "
        "uniform float aspectRatio; "
        "void main(){}";

    shadereditor::UniformIntrospectionService introspection;
    const auto uniforms = introspection.discover(document);

    REQUIRE(uniforms.size() == 3);
    REQUIRE(uniforms[0].name == "vpWidth");
    REQUIRE(uniforms[0].provenance == shadereditor::UniformProvenance::User);
    REQUIRE(uniforms[0].editable);
    REQUIRE(uniforms[1].name == "vpHeight");
    REQUIRE(uniforms[1].provenance == shadereditor::UniformProvenance::User);
    REQUIRE(uniforms[1].editable);
    REQUIRE(uniforms[2].name == "aspectRatio");
    REQUIRE(uniforms[2].provenance == shadereditor::UniformProvenance::PhoenixAuto);
    REQUIRE(!uniforms[2].editable);
}

TEST_CASE("uniform introspection excludes engine-owned MVP/uCameraPos/model uniforms entirely") {
    shadereditor::ShaderPairDocument document;
    // "model" (per-frame orbit rotation matrix) is engine-owned exactly like MVP/uCameraPos: it
    // must never be surfaced to the Uniforms panel at all (not even as a read-only row), since
    // the user has no meaningful value to inspect or edit for it.
    document.vertexSource = "uniform mat4 MVP; uniform vec3 uCameraPos; uniform mat4 model; uniform vec2 u_offset; void main(){}";

    shadereditor::UniformIntrospectionService introspection;
    const auto uniforms = introspection.discover(document);

    REQUIRE(uniforms.size() == 1);
    REQUIRE(uniforms.front().name == "u_offset");
}
