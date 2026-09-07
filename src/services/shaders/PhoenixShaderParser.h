#pragma once

#include "editor/ShaderPairDocument.h"

#include <string>

namespace shadereditor {
struct PhoenixParseResult {
    bool success {false};
    std::string errorMessage;
};

class PhoenixShaderParser {
  public:
    PhoenixParseResult parse(ShaderPairDocument& document) const;
};
}  // namespace shadereditor
