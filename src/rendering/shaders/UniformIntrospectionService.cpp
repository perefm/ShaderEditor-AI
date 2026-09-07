#include "rendering/shaders/UniformIntrospectionService.h"

#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <sstream>

namespace shadereditor {
namespace {
UniformDefinition makeUniformDefinition(const std::string& type, const std::string& name) {
    UniformDefinition uniform;
    uniform.name = name;
    uniform.kind = type;

    // Keep defaults useful for immediate preview feedback when the shader first loads.
    if (type == "bool") {
        uniform.componentCount = 1;
        uniform.defaultValue = UniformValue {false};
        uniform.currentValue = uniform.defaultValue;
        return uniform;
    }
    if (type == "int") {
        uniform.componentCount = 1;
        uniform.defaultValue = UniformValue {0};
        uniform.currentValue = uniform.defaultValue;
        return uniform;
    }
    if (type == "float") {
        uniform.componentCount = 1;
        uniform.defaultValue = UniformValue {0.0F};
        uniform.currentValue = uniform.defaultValue;
        return uniform;
    }
    if (type == "vec2") {
        uniform.componentCount = 2;
        uniform.defaultValue = UniformValue {glm::vec2(0.0F, 0.0F)};
        uniform.currentValue = uniform.defaultValue;
        return uniform;
    }
    if (type == "vec3") {
        uniform.componentCount = 3;
        uniform.defaultValue = UniformValue {glm::vec3(0.2F, 0.6F, 0.9F)};
        uniform.currentValue = uniform.defaultValue;
        return uniform;
    }
    if (type == "vec4") {
        uniform.componentCount = 4;
        uniform.defaultValue = UniformValue {glm::vec4(0.85F, 0.35F, 0.25F, 1.0F)};
        uniform.currentValue = uniform.defaultValue;
        return uniform;
    }
    if (type == "mat2") {
        uniform.componentCount = 4;
        uniform.defaultValue = UniformValue {glm::mat2(1.0F)};
        uniform.currentValue = uniform.defaultValue;
        return uniform;
    }
    if (type == "mat3") {
        uniform.componentCount = 9;
        uniform.defaultValue = UniformValue {glm::mat3(1.0F)};
        uniform.currentValue = uniform.defaultValue;
        return uniform;
    }
    if (type == "mat4") {
        uniform.componentCount = 16;
        uniform.defaultValue = UniformValue {glm::mat4(1.0F)};
        uniform.currentValue = uniform.defaultValue;
        return uniform;
    }

    uniform.editable = false;
    uniform.componentCount = 1;
    uniform.defaultValue = UniformValue {0.0F};
    uniform.currentValue = uniform.defaultValue;
    return uniform;
}

void collectUniforms(const std::string& source, std::vector<UniformDefinition>& uniforms) {
    std::istringstream input(source);
    std::string token;
    while (input >> token) {
        if (token == "uniform") {
            // The editor only needs simple token discovery for the current in-app preview workflow.
            std::string type;
            std::string name;
            input >> type >> name;
            if (!name.empty() && name.back() == ';') {
                name.pop_back();
            }
            uniforms.push_back(makeUniformDefinition(type, name));
        }
    }
}
}

std::vector<UniformDefinition> UniformIntrospectionService::discover(const ShaderPairDocument& document) const {
    std::vector<UniformDefinition> uniforms;
    collectUniforms(document.vertexSource, uniforms);
    collectUniforms(document.fragmentSource, uniforms);
    return uniforms;
}
}  // namespace shadereditor
