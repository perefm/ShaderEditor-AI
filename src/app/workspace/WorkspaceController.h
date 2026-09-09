#pragma once

#include "app/workspace/DiagnosticsState.h"
#include "app/workspace/PlaybackClockState.h"
#include "app/workspace/PreviewInteractionState.h"
#include "app/workspace/UniformState.h"
#include "editor/ShaderEditorState.h"
#include "rendering/models/AssimpModelLoader.h"
#include "rendering/models/ModelDocument.h"
#include "rendering/models/ModelInfoSummary.h"
#include "rendering/opengl/PreviewRenderer.h"
#include "rendering/shaders/RenderSession.h"
#include "rendering/shaders/UniformIntrospectionService.h"
#include "services/files/ShaderFileService.h"

#include <glm/vec4.hpp>

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

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
    void setRenderBackgroundColor(const glm::vec4& color);
    [[nodiscard]] glm::vec4 renderBackgroundColor() const { return renderSession_.backgroundColor; }

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
    // Names of the active model's animation clips (empty if no model is loaded or it has none),
    // for populating an animation-selection UI.
    [[nodiscard]] std::vector<std::string> modelAnimationNames() const { return previewRenderer_.activeModelAnimationNames(); }
    // Selects which animation clip SkeletalAnimator should play; -1 means "no animation" (bind
    // pose). Out-of-range indices are also treated as "no animation" by SkeletalAnimator.
    void selectAnimation(int animationIndex) { renderSession_.selectedAnimationIndex = animationIndex; }
    [[nodiscard]] int selectedAnimationIndex() const { return renderSession_.selectedAnimationIndex; }
    // Whether the active clip wraps or holds once playback time passes its duration.
    void setAnimationLooping(bool looping) {
        renderSession_.animationLoopMode = looping ? AnimationLoopMode::Loop : AnimationLoopMode::Hold;
    }
    [[nodiscard]] bool animationLooping() const { return renderSession_.animationLoopMode == AnimationLoopMode::Loop; }

    // Names of the cameras authored inside the active model, in the order their indices refer to.
    [[nodiscard]] std::vector<std::string> modelCameraNames() const { return previewRenderer_.activeModelCameraNames(); }
    // Chooses the camera the preview renders through: -1 is the free orbit/pan camera (Phoenix's
    // CameraNumber < 0), >= 0 selects a model camera. Out-of-range indices fall back to the free
    // camera so the selection can never point at a camera the current model does not have.
    void selectCamera(int cameraIndex);
    [[nodiscard]] int selectedCameraIndex() const { return renderSession_.activeCameraIndex; }
    // Statistics for the currently loaded model, refreshed on every successful openModel() and
    // cleared when a load fails, so the "Model info" panel never shows stale or partial data.
    [[nodiscard]] const ModelInfoSummary& modelInfo() const { return modelInfo_; }

    [[nodiscard]] ShaderEditorState& editorState() { return editorState_; }
    [[nodiscard]] const ShaderEditorState& editorState() const { return editorState_; }
    [[nodiscard]] const UniformState& uniformState() const { return uniformState_; }
    [[nodiscard]] const RenderSession& renderSession() const { return renderSession_; }

  private:
    void refreshUniforms();
    // True while a model-authored camera is driving the preview. Interaction is suppressed in
    // that case so orbit/pan/zoom cannot silently mutate the free camera's stored framing, which
    // must be restored untouched when the user switches back to it (FR-018).
    [[nodiscard]] bool usingSceneCamera() const {
        return renderSession_.renderTargetKind == RenderTargetKind::Model && renderSession_.activeCameraIndex >= 0;
    }

    DiagnosticsState& diagnostics_;
    ShaderFileService fileService_;
    UniformIntrospectionService introspectionService_;
    PreviewRenderer previewRenderer_;
    AssimpModelLoader modelLoader_;
    ShaderEditorState editorState_;
    UniformState uniformState_;
    RenderSession renderSession_;
    ModelInfoSummary modelInfo_;
    PlaybackClockState playbackClock_;
    RenderMetrics renderMetrics_;
    // Wall-clock timestamp of the previous renderPreview() call, used to compute the frame's
    // delta time for advancing the playback clock. Empty until the first frame is rendered.
    std::optional<std::chrono::steady_clock::time_point> lastFrameTime_;
    // Sampler2D (texture) assignments the user made by hand while a built-in primitive was the
    // render target, keyed by uniform name. Built-in primitives all share this single set (so
    // switching from a plane to a sphere keeps the same manually loaded texture, per the user's
    // request), while a loaded model keeps its own imported textures instead - see
    // selectPrimitive()/openModel()/selectModel(), which snapshot/restore these sets whenever the
    // render target kind changes so the two never bleed into each other.
    std::unordered_map<std::string, std::string> primitiveTextureValues_;
};
}  // namespace shadereditor
