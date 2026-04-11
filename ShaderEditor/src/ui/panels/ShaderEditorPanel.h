#pragma once

#include "app/workspace/WorkspaceController.h"

namespace shadereditor {
class ShaderEditorPanel {
  public:
    explicit ShaderEditorPanel(WorkspaceController& controller);

    bool pressUpdateButton();
    bool pressCtrlEnter();

  private:
    WorkspaceController& controller_;
};
}  // namespace shadereditor
