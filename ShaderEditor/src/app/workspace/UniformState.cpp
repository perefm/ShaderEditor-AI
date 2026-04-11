#include "app/workspace/UniformState.h"

namespace shadereditor {
void UniformState::setDefinitions(const std::vector<UniformDefinition>& definitions) { definitions_ = definitions; }

void UniformState::apply(const std::string& name, UniformValue value) {
    for (auto& definition : definitions_) {
        if (definition.name == name) {
            definition.currentValue = std::move(value);
            break;
        }
    }
}

std::unordered_map<std::string, UniformValue> UniformState::values() const {
    std::unordered_map<std::string, UniformValue> result;
    for (const auto& definition : definitions_) {
        result[definition.name] = definition.currentValue;
    }
    return result;
}
}  // namespace shadereditor
