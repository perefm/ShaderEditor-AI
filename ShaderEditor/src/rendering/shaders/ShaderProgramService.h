#pragma once

#include "editor/ShaderPairDocument.h"

#include <string>

namespace shadereditor {
// Result of validating the current shader pair through the lightweight compiler service.
struct ShaderCompileResult {
    bool success {false};
    std::string errorMessage;
};

// Performs source-level sanity checks before the OpenGL renderer compiles the program for real.
class ShaderProgramService {
  public:
    ShaderCompileResult compileAndLink(const ShaderPairDocument& document) const;
};
}  // namespace shadereditor
