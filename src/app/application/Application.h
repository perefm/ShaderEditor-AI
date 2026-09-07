#pragma once

#include "app/platform/WindowContext.h"
#include "app/workspace/DiagnosticsState.h"
#include "app/workspace/WorkspaceController.h"
#include "ui/dockspace/DockspaceHost.h"
#include "ui/panels/DiagnosticsPanel.h"
#include "ui/panels/RenderViewPanel.h"
#include "ui/panels/ShaderEditorPanel.h"
#include "ui/panels/ShaderErrorsPanel.h"
#include "ui/panels/UniformsPanel.h"
#include "ui/widgets/DocumentDialogs.h"
#include "imgui_color_text_edit/TextEditor.h"

#include <filesystem>

namespace shadereditor {
// Owns the window lifetime and renders all top-level dockable panels.
class Application {
  public:
    Application();
    bool initialize();
    int run();
    void shutdown();

    [[nodiscard]] WorkspaceController& workspace() { return workspace_; }

  private:
    // Example shaders live under the runtime assets folder next to the executable.
    bool loadExampleShaders();
    bool openShaderFromDialog();
    bool saveShaderAsFromDialog();
    bool openImageForUniform(const std::string& uniformName);
    // Prompts for a 3D model file (glTF/GLB/FBX/OBJ/etc., anything Assimp supports) and imports
    // it as the new preview render target.
    bool openModelFromDialog();
    // Each major tool panel is drawn independently so ImGui docking can rearrange them.
    void drawMainMenu();
    void drawWorkspaceHost();
    void drawShaderEditorWindow();
    void drawRenderViewWindow();
    void drawUniformsWindow();
    void drawDiagnosticsWindow();
    void drawShaderErrorsWindow();
    void drawShaderHelpWindow();
    void registerPanels();
    void drawUi();

    // Native window + OpenGL context bootstrap.
    WindowContext windowContext_;
    // Shared log stream rendered by the diagnostics UI.
    DiagnosticsState diagnostics_;
    // Central document, uniform and preview state.
    WorkspaceController workspace_;
    DockspaceHost dockspaceHost_;
    ShaderEditorPanel shaderEditorPanel_;
    RenderViewPanel renderViewPanel_;
    UniformsPanel uniformsPanel_;
    DiagnosticsPanel diagnosticsPanel_;
    ShaderErrorsPanel shaderErrorsPanel_;
    DocumentDialogs documentDialogs_;
    // Runtime paths for the bundled example shaders.
    std::filesystem::path exampleShaderPath_;
    std::filesystem::path pixelLightingShaderPath_;
    bool showShaderEditor_ {true};
    bool showRenderView_ {true};
    bool showUniforms_ {true};
    bool showDiagnostics_ {false};
    bool showShaderErrors_ {true};
    bool showShaderHelp_ {false};
    TextEditor shaderEditor_;
    std::string shaderEditorText_;
    // Dear ImGui persists its dock layout beside the executable, never in the runtime assets tree.
    std::string imguiIniPath_;
};
}  // namespace shadereditor
