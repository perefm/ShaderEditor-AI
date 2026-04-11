#pragma once

#include "app/workspace/DiagnosticsState.h"

namespace shadereditor {
class DiagnosticsPanel {
  public:
    explicit DiagnosticsPanel(const DiagnosticsState& diagnostics);
    [[nodiscard]] const std::vector<std::string>& messages() const;

  private:
    const DiagnosticsState& diagnostics_;
};
}  // namespace shadereditor
