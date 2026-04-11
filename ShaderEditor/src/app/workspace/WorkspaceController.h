#pragma once

#include "app/workspace/DiagnosticsState.h"
#include "app/workspace/PreviewInteractionState.h"
#include "app/workspace/UniformState.h"
#include "editor/ShaderEditorState.h"
#include "rendering/opengl/PreviewRenderer.h"
#include "rendering/shaders/RenderSession.h"
#include "rendering/shaders/UniformIntrospectionService.h"
#include "services/files/ShaderFileService.h"

#include <filesystem>

namespace shadereditor {
class WorkspaceController {
  public:
    explicit WorkspaceController(DiagnosticsState& diagnostics);

    bool openShaders(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
    bool openVertexShader(const std::filesystem::path& vertexPath);
    bool openFragmentShader(const std::filesystem::path& fragmentPath);
    bool saveShaders();
    void discardUnsavedChanges();
    bool updateShaders();
    const RenderSession& renderPreview(int width, int height);
    bool handleKeyChord(const std::string& chord);
    void selectPrimitive(const std::string& primitiveId);
    void applyUniform(const std::string& name, UniformValue value);
    void orbitPreview(const glm::vec2& delta);
    void panPreview(const glm::vec2& delta);
    void resetPreviewInteraction();

    [[nodiscard]] ShaderEditorState& editorState() { return editorState_; }
    [[nodiscard]] const ShaderEditorState& editorState() const { return editorState_; }
    [[nodiscard]] const UniformState& uniformState() const { return uniformState_; }
    [[nodiscard]] const RenderSession& renderSession() const { return renderSession_; }

  private:
    void refreshUniforms();

    DiagnosticsState& diagnostics_;
    ShaderFileService fileService_;
    UniformIntrospectionService introspectionService_;
    PreviewRenderer previewRenderer_;
    ShaderEditorState editorState_;
    UniformState uniformState_;
    RenderSession renderSession_;
};
}  // namespace shadereditor
