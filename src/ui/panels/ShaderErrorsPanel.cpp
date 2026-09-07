#include "ui/panels/ShaderErrorsPanel.h"

namespace shadereditor {
ShaderErrorsPanel::ShaderErrorsPanel(const DiagnosticsState& diagnostics) : diagnostics_(diagnostics) {}

const std::vector<std::string>& ShaderErrorsPanel::errors() const {
    // Shader errors stay in their own panel so failed compiles do not get buried in general diagnostics.
    return diagnostics_.shaderErrors();
}
}  // namespace shadereditor
