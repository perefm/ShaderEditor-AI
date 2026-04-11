#pragma once

#include "rendering/shaders/UniformDefinition.h"

#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <unordered_map>
#include <vector>

namespace shadereditor {
// Holds the editable uniform set currently exposed to the UI.
class UniformState {
  public:
    void setDefinitions(const std::vector<UniformDefinition>& definitions);
    void apply(const std::string& name, UniformValue value);
    [[nodiscard]] const std::vector<UniformDefinition>& definitions() const { return definitions_; }
    [[nodiscard]] std::unordered_map<std::string, UniformValue> values() const;

  private:
    std::vector<UniformDefinition> definitions_;
};
}  // namespace shadereditor
