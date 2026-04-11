#include "ui/panels/UniformsPanel.h"

namespace shadereditor {
UniformsPanel::UniformsPanel(WorkspaceController& controller) : controller_(controller) {}

void UniformsPanel::setFloat(const std::string& name, float value) {
    // The panel forwards scalar changes through the shared workspace command path.
    controller_.applyUniform(name, value);
}
}  // namespace shadereditor
