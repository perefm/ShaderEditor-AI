#include "ui/panels/RenderViewPanel.h"

namespace shadereditor {
RenderViewPanel::RenderViewPanel(WorkspaceController& controller) : controller_(controller) {}

void RenderViewPanel::choosePrimitive(const std::string& primitiveId) { controller_.selectPrimitive(primitiveId); }

void RenderViewPanel::orbit(const glm::vec2& delta) {
    // Panel-level interaction methods keep the application shell free from workspace internals.
    controller_.orbitPreview(delta);
}

void RenderViewPanel::pan(const glm::vec2& delta) { controller_.panPreview(delta); }

void RenderViewPanel::zoom(float wheelDelta) { controller_.zoomPreview(wheelDelta); }

void RenderViewPanel::resetView() { controller_.resetPreviewInteraction(); }

const RenderSession& RenderViewPanel::renderPreview(int width, int height) { return controller_.renderPreview(width, height); }

std::string RenderViewPanel::summary() const { return controller_.renderSession().previewSummary; }

std::string RenderViewPanel::errorMessage() const { return controller_.renderSession().errorMessage; }
}  // namespace shadereditor
