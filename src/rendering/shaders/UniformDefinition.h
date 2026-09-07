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

// Metadata discovered from shader text and consumed by both UI controls and GL uploads.
struct UniformDefinition {
    std::string name;
    std::string kind {"float"};
    int componentCount {1};
    UniformValue defaultValue {0.0F};
    UniformValue currentValue {0.0F};
    bool editable {true};
    std::string validationRule;
};
}  // namespace shadereditor

