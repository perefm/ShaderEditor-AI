#pragma once

#include "app/workspace/WorkspaceController.h"

namespace shadereditor {
// Wraps editor-triggered commands so the UI stays thin.
class ShaderEditorPanel {
  public:
    explicit ShaderEditorPanel(WorkspaceController& controller);

    bool pressUpdateButton();
    bool pressCtrlEnter();

  private:
    WorkspaceController& controller_;
};
}  // namespace shadereditor
