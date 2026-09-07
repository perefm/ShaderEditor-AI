#pragma once

#include "app/workspace/DiagnosticsState.h"
#include "app/workspace/PlaybackClockState.h"
#include "app/workspace/PreviewInteractionState.h"
#include "app/workspace/UniformState.h"
#include "editor/ShaderEditorState.h"
#include "rendering/models/AssimpModelLoader.h"
#include "rendering/models/ModelDocument.h"
#include "rendering/opengl/PreviewRenderer.h"
#include "rendering/shaders/RenderSession.h"
#include "rendering/shaders/UniformIntrospectionService.h"
#include "services/files/ShaderFileService.h"

#include <chrono>
#include <filesystem>
#include <optional>

namespace shadereditor {
// Coordinates document loading, uniform discovery and preview rendering for the UI layer.
class WorkspaceController {
  public:
    explicit WorkspaceController(DiagnosticsState& diagnostics);

    // File operations update the active document and rebuild dependent state when possible.
    bool openShader(const std::filesystem::path& shaderPath);
    bool openShaders(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
    bool openVertexShader(const std::filesystem::path& vertexPath);
    bool openFragmentShader(const std::filesystem::path& fragmentPath);
    bool saveShaders();
    // Writes the active document to a new file path and re-targets it there; returns false
    // (leaving the document untouched) on I/O failure, mirroring saveShaders().
    bool saveShadersAs(const std::filesystem::path& newPath);
    void discardUnsavedChanges();
    // updateShaders refreshes the logical render session; actual drawing happens on demand.
    bool updateShaders();
    // Advances the playback clock (by the real time elapsed since the previous call) and renders.
    const RenderSession& renderPreview(int width, int height);
    bool handleKeyChord(const std::string& chord);
    // Preview interaction flows through the workspace so panels do not manipulate render state directly.
    void selectPrimitive(const std::string& primitiveId);
    void applyUniform(const std::string& name, UniformValue value);
    void orbitPreview(const glm::vec2& delta);
    void panPreview(const glm::vec2& delta);
    void zoomPreview(float wheelDelta);
    void resetPreviewInteraction();

    // Playback transport controls for Phoenix's time-based auto-uniforms ("t"/"tend"/"beat").
    void playPreview();
    void pausePreview();
    void resetPreview();
    void setSectionDuration(float seconds);
    void setBpm(float bpm);
    [[nodiscard]] const PlaybackClockState& playbackClock() const { return playbackClock_; }

    // Imports a 3D model via Assimp and makes it the active render target. On failure, records
    // the error via diagnostics_ and leaves any previously loaded model/render target untouched
    // (FR-019). Returns true on success.
    bool openModel(const std::filesystem::path& modelPath);
    // Switches the preview back to drawing the given built-in primitive (see selectPrimitive
    // above, which is retained for this purpose) - kept here only as a doc pointer; use
    // selectPrimitive() to leave model mode. A previously loaded model stays cached so switching
    // back to it via selectModel() does not require re-importing.
    // Re-activates the currently cached model (if any) as the render target, without
    // re-importing it. No-op (returns false) if no model has ever been loaded successfully.
    bool selectModel();
    [[nodiscard]] bool hasLoadedModel() const { return previewRenderer_.hasLoadedModel(); }

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
    AssimpModelLoader modelLoader_;
    ShaderEditorState editorState_;
    UniformState uniformState_;
    RenderSession renderSession_;
    PlaybackClockState playbackClock_;
    // Wall-clock timestamp of the previous renderPreview() call, used to compute the frame's
    // delta time for advancing the playback clock. Empty until the first frame is rendered.
    std::optional<std::chrono::steady_clock::time_point> lastFrameTime_;
};
}  // namespace shadereditor
