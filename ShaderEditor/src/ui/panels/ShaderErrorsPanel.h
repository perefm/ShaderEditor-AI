#pragma once

#include "app/workspace/DiagnosticsState.h"

namespace shadereditor {
class ShaderErrorsPanel {
  public:
    explicit ShaderErrorsPanel(const DiagnosticsState& diagnostics);
    [[nodiscard]] const std::vector<std::string>& errors() const;

  private:
    const DiagnosticsState& diagnostics_;
};
}  // namespace shadereditor
