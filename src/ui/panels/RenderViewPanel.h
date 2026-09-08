#pragma once

#include "app/workspace/WorkspaceController.h"

#include <glm/vec2.hpp>
#include <string>
#include <vector>

namespace shadereditor {
// Thin UI-facing wrapper around preview rendering and interaction actions.
class RenderViewPanel {
  public:
    explicit RenderViewPanel(WorkspaceController& controller);

    void choosePrimitive(const std::string& primitiveId);
    void orbit(const glm::vec2& delta);
    void pan(const glm::vec2& delta);
    void zoom(float wheelDelta);
    void resetView();
    const RenderSession& renderPreview(int width, int height);
    [[nodiscard]] std::string summary() const;
    [[nodiscard]] std::string errorMessage() const;
    // Animation selection passthroughs for the model animation picker (see RenderView panel UI).
    [[nodiscard]] std::vector<std::string> animationNames() const { return controller_.modelAnimationNames(); }
    void selectAnimation(int animationIndex) { controller_.selectAnimation(animationIndex); }
    [[nodiscard]] int selectedAnimationIndex() const { return controller_.selectedAnimationIndex(); }
    void setAnimationLooping(bool looping) { controller_.setAnimationLooping(looping); }
    [[nodiscard]] bool animationLooping() const { return controller_.animationLooping(); }
    // Camera selection passthroughs: -1 is the free orbit camera, >= 0 a model-authored camera.
    [[nodiscard]] std::vector<std::string> cameraNames() const { return controller_.modelCameraNames(); }
    void selectCamera(int cameraIndex) { controller_.selectCamera(cameraIndex); }
    [[nodiscard]] int selectedCameraIndex() const { return controller_.selectedCameraIndex(); }

  private:
    WorkspaceController& controller_;
};
}  // namespace shadereditor
