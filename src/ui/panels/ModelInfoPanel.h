#pragma once

#include "app/workspace/WorkspaceController.h"

namespace shadereditor {
// Read-only "Model info" panel: renders the statistics of the currently loaded model. Holds no
// state of its own and never mutates render/camera/animation/shader state, so opening it can
// never change what the preview shows.
class ModelInfoPanel {
  public:
    explicit ModelInfoPanel(WorkspaceController& controller);

    // Draws the panel body (the caller owns the ImGui::Begin/End window). The summary is whatever
    // WorkspaceController cached during the last successful model import, so it refreshes
    // automatically each time a model is loaded.
    void draw() const;

  private:
    WorkspaceController& controller_;
};
}  // namespace shadereditor
