#pragma once

#include "editor/ShaderPairDocument.h"
#include "rendering/shaders/UniformDefinition.h"

#include <vector>

namespace shadereditor {
// Extracts editable uniform metadata from shader source code.
class UniformIntrospectionService {
  public:
    std::vector<UniformDefinition> discover(const ShaderPairDocument& document) const;
};
}  // namespace shadereditor
