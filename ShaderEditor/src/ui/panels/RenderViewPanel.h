#pragma once

#include "app/workspace/WorkspaceController.h"

#include <glm/vec2.hpp>
#include <string>

namespace shadereditor {
class RenderViewPanel {
  public:
    explicit RenderViewPanel(WorkspaceController& controller);

    void choosePrimitive(const std::string& primitiveId);
    void orbit(const glm::vec2& delta);
    void pan(const glm::vec2& delta);
    void resetView();
    const RenderSession& renderPreview(int width, int height);
    [[nodiscard]] std::string summary() const;
    [[nodiscard]] std::string errorMessage() const;

  private:
    WorkspaceController& controller_;
};
}  // namespace shadereditor
