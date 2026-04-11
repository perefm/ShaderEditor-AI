#pragma once

#include "app/platform/WindowContext.h"
#include "app/workspace/DiagnosticsState.h"
#include "app/workspace/WorkspaceController.h"
#include "app/workspace/WorkspaceLayoutState.h"
#include "services/persistence/LayoutPersistenceService.h"
#include "ui/dockspace/DockspaceHost.h"
#include "ui/panels/DiagnosticsPanel.h"
#include "ui/panels/RenderViewPanel.h"
#include "ui/panels/ShaderEditorPanel.h"
#include "ui/panels/ShaderErrorsPanel.h"
#include "ui/panels/UniformsPanel.h"
#include "ui/widgets/DocumentDialogs.h"

#include <filesystem>

namespace shadereditor {
class Application {
  public:
    Application();
    bool initialize();
    int run();
    void shutdown();

    [[nodiscard]] WorkspaceController& workspace() { return workspace_; }

  private:
    bool loadExampleShaders();
    bool openVertexShaderFromDialog();
    bool openFragmentShaderFromDialog();
    void restorePanelVisibility();
    void storePanelVisibility();
    void drawMainMenu();
    void drawWorkspaceHost();
    void drawShaderEditorWindow();
    void drawRenderViewWindow();
    void drawUniformsWindow();
    void drawDiagnosticsWindow();
    void drawShaderErrorsWindow();
    void registerPanels();
    void drawUi();

    WindowContext windowContext_;
    DiagnosticsState diagnostics_;
    WorkspaceController workspace_;
    WorkspaceLayoutState layoutState_;
    LayoutPersistenceService layoutPersistence_;
    DockspaceHost dockspaceHost_;
    ShaderEditorPanel shaderEditorPanel_;
    RenderViewPanel renderViewPanel_;
    UniformsPanel uniformsPanel_;
    DiagnosticsPanel diagnosticsPanel_;
    ShaderErrorsPanel shaderErrorsPanel_;
    DocumentDialogs documentDialogs_;
    std::filesystem::path exampleVertexPath_;
    std::filesystem::path exampleFragmentPath_;
    bool showShaderEditor_ {true};
    bool showRenderView_ {true};
    bool showUniforms_ {true};
    bool showDiagnostics_ {true};
    bool showShaderErrors_ {true};
};
}  // namespace shadereditor
