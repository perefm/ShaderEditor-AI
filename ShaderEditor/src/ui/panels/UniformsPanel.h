#pragma once

#include "app/workspace/WorkspaceController.h"

namespace shadereditor {
class UniformsPanel {
  public:
    explicit UniformsPanel(WorkspaceController& controller);
    void setFloat(const std::string& name, float value);

  private:
    WorkspaceController& controller_;
};
}  // namespace shadereditor
