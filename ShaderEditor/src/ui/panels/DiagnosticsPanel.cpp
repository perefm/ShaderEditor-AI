#include "ui/panels/DiagnosticsPanel.h"

namespace shadereditor {
DiagnosticsPanel::DiagnosticsPanel(const DiagnosticsState& diagnostics) : diagnostics_(diagnostics) {}

const std::vector<std::string>& DiagnosticsPanel::messages() const {
    // Diagnostics are rendered as a read-only projection of the shared workspace state.
    return diagnostics_.messages();
}
}  // namespace shadereditor
