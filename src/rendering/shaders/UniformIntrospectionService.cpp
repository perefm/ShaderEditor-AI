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
// Phoenix engine auto-uniforms recognized by name: their value is supplied automatically by
// ShaderEditor (playback clock, active mesh material, bone animator) rather than by the user.
// Each entry pairs the exact Phoenix uniform name with the GLSL type it must declare to be
// treated as auto-populated; any other type for that name is left as an ordinary user uniform
// (see data-model.md's "Recognized auto-uniform table").
bool isPhoenixAutoUniform(const std::string& name, const std::string& type) {
    if ((name == "t" || name == "tend" || name == "beat") && type == "float") {
        return true;
    }
    if ((name == "Mat_Ka" || name == "Mat_Kd" || name == "Mat_Ks") && type == "vec3") {
        return true;
    }
    if (name == "Mat_KsStrenght" && type == "float") {
        return true;
    }
    // "gBones" is always an array (e.g. "uniform mat4 gBones[100];"); the simple tokenizer below
    // strips the "[...]" suffix from the name before this check runs, so only the type matters here.
    if (name == "gBones" && type == "mat4") {
        return true;
    }
    return false;
}

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
        uniform.defaultValue = UniformValue {0.5F};
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
    if (type == "sampler2D") {
        uniform.componentCount = 1;
        uniform.defaultValue = UniformValue {std::string {}};
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
            // Strip an array suffix (e.g. "gBones[100]" -> "gBones") so array declarations like
            // Phoenix's "uniform mat4 gBones[100];" are matched by plain name/type comparisons.
            const auto bracketPosition = name.find('[');
            if (bracketPosition != std::string::npos) {
                name.erase(bracketPosition);
            }
            if (name == "MVP" || name == "uCameraPos" || name == "model") {
                continue;
            }
            UniformDefinition uniform = makeUniformDefinition(type, name);
            if (isPhoenixAutoUniform(name, type)) {
                // Auto-uniforms are computed by the engine every frame, so the Uniforms panel
                // must never let the user edit them directly (FR-012).
                uniform.provenance = UniformProvenance::PhoenixAuto;
                uniform.editable = false;
            }
            uniforms.push_back(std::move(uniform));
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
