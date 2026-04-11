#pragma once

#include "editor/ShaderPairDocument.h"

#include <string>

namespace shadereditor {
struct ShaderCompileResult {
    bool success {false};
    std::string errorMessage;
};

class ShaderProgramService {
  public:
    ShaderCompileResult compileAndLink(const ShaderPairDocument& document) const;
};
}  // namespace shadereditor
