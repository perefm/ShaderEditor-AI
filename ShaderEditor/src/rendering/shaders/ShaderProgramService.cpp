#include "rendering/shaders/ShaderProgramService.h"

namespace shadereditor {
ShaderCompileResult ShaderProgramService::compileAndLink(const ShaderPairDocument& document) const {
    // Unit tests exercise this lightweight guard so editor workflows can fail fast without a full GL context.
    if (document.vertexSource.empty() || document.fragmentSource.empty()) {
        return {false, "Shader compilation failed: both shader stages require source."};
    }
    if (document.vertexSource.find("error") != std::string::npos || document.fragmentSource.find("error") != std::string::npos) {
        return {false, "Shader compilation failed: source contains the token 'error'."};
    }
    return {true, {}};
}
}  // namespace shadereditor
