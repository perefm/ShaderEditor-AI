#include "ui/panels/ShaderEditorPanel.h"

namespace shadereditor {
ShaderEditorPanel::ShaderEditorPanel(WorkspaceController& controller) : controller_(controller) {}

bool ShaderEditorPanel::pressUpdateButton() {
    // Button clicks and keyboard shortcuts share the same workspace update entry point.
    return controller_.updateShaders();
}

bool ShaderEditorPanel::pressCtrlEnter() { return controller_.handleKeyChord("Ctrl+Enter"); }
}  // namespace shadereditor
