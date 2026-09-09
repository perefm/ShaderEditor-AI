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
    // Resets every sampler2D (texture) uniform back to its empty default. Used together with
    // captureTextureValues()/restoreTextureValues() below so the render target switch that
    // triggers a reset can save off the outgoing target's texture assignments first.
    void clearTextureValues();
    // Returns the current value of every sampler2D uniform, keyed by name. WorkspaceController
    // uses this to stash "model" and "primitive" texture assignments independently when the
    // render target changes, so switching between built-in primitives keeps the primitives'
    // shared textures, switching to a model shows the model's own textures, and switching back
    // restores whichever set was active before - each render-target kind keeps its own textures.
    [[nodiscard]] std::unordered_map<std::string, std::string> captureTextureValues() const;
    // Applies previously captured sampler2D values back onto the matching uniforms by name.
    // Uniforms with no entry in `values` (e.g. a sampler the new shader doesn't declare, or one
    // that didn't exist when the values were captured) are left at their current value, and any
    // sampler2D uniform not covered by `values` is left untouched by this call - callers that
    // want a clean slate should call clearTextureValues() first.
    void restoreTextureValues(const std::unordered_map<std::string, std::string>& values);
    [[nodiscard]] const std::vector<UniformDefinition>& definitions() const { return definitions_; }
    [[nodiscard]] std::unordered_map<std::string, UniformValue> values() const;

  private:
    std::vector<UniformDefinition> definitions_;
};
}  // namespace shadereditor
