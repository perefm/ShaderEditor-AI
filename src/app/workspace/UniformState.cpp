#include "app/workspace/UniformState.h"

namespace shadereditor {
// Replace the editable uniform list with the latest discovery result from the active shader pair.
void UniformState::setDefinitions(const std::vector<UniformDefinition>& definitions) { definitions_ = definitions; }

void UniformState::apply(const std::string& name, UniformValue value) {
    // The state is keyed by name because the UI sends updates for individual uniforms.
    for (auto& definition : definitions_) {
        if (definition.name == name) {
            definition.currentValue = std::move(value);
            break;
        }
    }
}

std::unordered_map<std::string, UniformValue> UniformState::values() const {
    std::unordered_map<std::string, UniformValue> result;
    // Build a lookup map for render-time uniform uploads.
    for (const auto& definition : definitions_) {
        result[definition.name] = definition.currentValue;
    }
    return result;
}
}  // namespace shadereditor
