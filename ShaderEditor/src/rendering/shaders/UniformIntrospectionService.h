#pragma once

#include "editor/ShaderPairDocument.h"
#include "rendering/shaders/UniformDefinition.h"

#include <vector>

namespace shadereditor {
class UniformIntrospectionService {
  public:
    std::vector<UniformDefinition> discover(const ShaderPairDocument& document) const;
};
}  // namespace shadereditor
