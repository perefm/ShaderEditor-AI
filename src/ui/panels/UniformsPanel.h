#pragma once

#include "app/workspace/WorkspaceController.h"

namespace shadereditor {
// Small façade for uniform edits initiated from the UI.
class UniformsPanel {
  public:
    explicit UniformsPanel(WorkspaceController& controller);
    void setFloat(const std::string& name, float value);

  private:
    WorkspaceController& controller_;
};
}  // namespace shadereditor
