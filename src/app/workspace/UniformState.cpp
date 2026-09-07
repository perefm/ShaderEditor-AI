#include "app/workspace/UniformState.h"

namespace shadereditor {
// Replace the editable uniform list while preserving compatible values from the prior shader.
void UniformState::setDefinitions(const std::vector<UniformDefinition>& definitions) {
    std::unordered_map<std::string, UniformValue> previousValues;
    std::unordered_map<std::string, std::string> previousKinds;
    for (const auto& definition : definitions_) {
        previousValues[definition.name] = definition.currentValue;
        previousKinds[definition.name] = definition.kind;
    }

    definitions_ = definitions;
    for (auto& definition : definitions_) {
        const auto value = previousValues.find(definition.name);
        const auto kind = previousKinds.find(definition.name);
        if (value != previousValues.end() && kind != previousKinds.end() && kind->second == definition.kind) {
            definition.currentValue = value->second;
        }
    }
}

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
