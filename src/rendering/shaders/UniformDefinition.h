#pragma once

#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>
#include <variant>
#include <vector>

namespace shadereditor {
// Union of all GLSL uniform shapes that the editor can inspect and modify.
using UniformValue = std::variant<int, bool, float, glm::vec2, glm::vec3, glm::vec4, glm::mat2, glm::mat3, glm::mat4, std::string>;

// Distinguishes uniforms the user edits by hand from ones ShaderEditor computes and supplies
// automatically (Phoenix engine variables such as "t"/"tend"/"beat", material colors, bones).
enum class UniformProvenance { User, PhoenixAuto };

// Metadata discovered from shader text and consumed by both UI controls and GL uploads.
struct UniformDefinition {
    std::string name;
    std::string kind {"float"};
    int componentCount {1};
    UniformValue defaultValue {0.0F};
    UniformValue currentValue {0.0F};
    bool editable {true};
    std::string validationRule;
    // Set to PhoenixAuto when this uniform is recognized as one of Phoenix's auto-populated
    // engine variables; such uniforms are never user-editable (editable is forced to false)
    // and their value is overwritten every frame from the relevant engine-state source
    // (playback clock, active mesh material, bone animator) rather than from user input.
    UniformProvenance provenance {UniformProvenance::User};
};
}  // namespace shadereditor

