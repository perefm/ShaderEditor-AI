#include "app/workspace/WorkspaceController.h"

#include <algorithm>
#include <utility>

namespace shadereditor {
WorkspaceController::WorkspaceController(DiagnosticsState& diagnostics) : diagnostics_(diagnostics) {}

bool WorkspaceController::openShader(const std::filesystem::path& shaderPath) {
    try {
        editorState_.attachDocument(fileService_.load(shaderPath));
        diagnostics_.addInfo("Loaded Phoenix shader.");
        return updateShaders();
    } catch (const std::exception& ex) {
        diagnostics_.addError(ex.what());
        return false;
    }
}
bool WorkspaceController::openShaders(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) {
    try {
        // Loading the shader pair together gives the preview everything it needs in one pass.
        editorState_.attachDocument(fileService_.load(vertexPath, fragmentPath));
        diagnostics_.addInfo("Loaded shader pair.");
        return updateShaders();
    } catch (const std::exception& ex) {
        diagnostics_.addError(ex.what());
        return false;
    }
}

bool WorkspaceController::openVertexShader(const std::filesystem::path& shaderPath) {
    return openShader(shaderPath);
}

bool WorkspaceController::openFragmentShader(const std::filesystem::path& shaderPath) {
    return openShader(shaderPath);
}
bool WorkspaceController::saveShaders() {
    try {
        fileService_.save(editorState_.document());
        diagnostics_.addInfo("Saved shader pair.");
        return true;
    } catch (const std::exception& ex) {
        diagnostics_.addError(ex.what());
        return false;
    }
}

void WorkspaceController::discardUnsavedChanges() { editorState_.document().isDirty = false; }

bool WorkspaceController::saveShadersAs(const std::filesystem::path& newPath) {
    try {
        fileService_.saveAs(editorState_.document(), newPath);
        diagnostics_.addInfo("Saved shader as " + newPath.string() + ".");
        return true;
    } catch (const std::exception& ex) {
        diagnostics_.addError(ex.what());
        return false;
    }
}

bool WorkspaceController::updateShaders() {
    diagnostics_.clearShaderErrors();
    // Compile first so a failed attempt cannot replace the last valid uniform state.
    const auto previousInteraction = renderSession_.interactionState;
    const auto previousPrimitive = renderSession_.selectedPrimitiveId;
    // updatePreview() always builds a fresh RenderSession, so the model-vs-primitive render
    // target and loaded-model id must be carried across the call explicitly (mirrors how
    // previousInteraction is restored below).
    const auto previousRenderTargetKind = renderSession_.renderTargetKind;
    const auto previousLoadedModelId = renderSession_.loadedModelId;
    const auto previousSelectedAnimationIndex = renderSession_.selectedAnimationIndex;
    const auto previousBackgroundColor = renderSession_.backgroundColor;
    const auto nextSession = previewRenderer_.updatePreview(editorState_.document(), previousPrimitive, uniformState_.definitions());
    if (nextSession.frameStatus == FrameStatus::Error) {
        renderSession_.errorMessage = nextSession.errorMessage;
        diagnostics_.addError(renderSession_.errorMessage);
        return false;
    }

    renderSession_ = nextSession;
    renderSession_.interactionState = previousInteraction;
    renderSession_.renderTargetKind = previousRenderTargetKind;
    renderSession_.loadedModelId = previousLoadedModelId;
    renderSession_.selectedAnimationIndex = previousSelectedAnimationIndex;
    renderSession_.backgroundColor = previousBackgroundColor;
    refreshUniforms();
    diagnostics_.addInfo("Updated shader preview.");
    return true;
}

const RenderSession& WorkspaceController::renderPreview(int width, int height) {
    // Compute this frame's delta time from real elapsed wall-clock time so the playback clock
    // advances at the same rate regardless of the UI's frame rate.
    const auto now = std::chrono::steady_clock::now();
    if (lastFrameTime_) {
        const std::chrono::duration<float> delta = now - *lastFrameTime_;
        playbackClock_.advance(delta.count());
        renderMetrics_.recordFrame(delta.count());
    }
    lastFrameTime_ = now;

    renderSession_.viewport = makeViewportMetrics(width, height);
    renderSession_.previewWidth = renderSession_.viewport.widthPixels;
    renderSession_.previewHeight = renderSession_.viewport.heightPixels;
    renderSession_.displayFps = renderMetrics_.displayFps();
    // Publish the up-to-date clock snapshot before rendering so PreviewRenderer can use it to
    // override t/tend/beat uniform values for this frame.
    renderSession_.playback = playbackClock_;
    // Rendering is lazy so frames that only rearrange UI do not rebuild the preview texture needlessly.
    renderSession_ = previewRenderer_.renderFrame(editorState_.document(), renderSession_, uniformState_.definitions(), width, height);
    renderSession_.playback = playbackClock_;
    return renderSession_;
}

void WorkspaceController::playPreview() { playbackClock_.play(); }

void WorkspaceController::pausePreview() { playbackClock_.pause(); }

void WorkspaceController::resetPreview() { playbackClock_.reset(); }

void WorkspaceController::setSectionDuration(float seconds) { playbackClock_.setSectionDurationSeconds(seconds); }

void WorkspaceController::setBpm(float bpm) { playbackClock_.setBpm(bpm); }

void WorkspaceController::setRenderBackgroundColor(const glm::vec4& color) {
    renderSession_.backgroundColor = glm::vec4(
        std::clamp(color.r, 0.0F, 1.0F),
        std::clamp(color.g, 0.0F, 1.0F),
        std::clamp(color.b, 0.0F, 1.0F),
        std::clamp(color.a, 0.0F, 1.0F));
}

bool WorkspaceController::handleKeyChord(const std::string& chord) { return chord == "Ctrl+Enter" ? updateShaders() : false; }

void WorkspaceController::selectPrimitive(const std::string& primitiveId) {
    // Switching back to a primitive leaves any loaded model cached in previewRenderer_ (see
    // selectModel()) so the user can flip back and forth without re-importing.
    renderSession_.renderTargetKind = RenderTargetKind::Primitive;
    // Primitive changes go through the same update path as shader edits to keep diagnostics unified.
    renderSession_.selectedPrimitiveId = primitiveId;
    // Built-in primitives are ~1 unit across, so restore the default (non-model) camera scale;
    // selectModel() below restores the model's own scale when switching back to it.
    renderSession_.interactionState.setSceneScale(1.0F);
    updateShaders();
}

bool WorkspaceController::openModel(const std::filesystem::path& modelPath) {
    const ModelLoadResult result = modelLoader_.load(modelPath);
    if (!result.success) {
        diagnostics_.addError("Failed to import model: " + result.errorMessage);
        return false;
    }
    // Capture the bounding radius before the document is moved into previewRenderer_, so the
    // preview camera can be reframed proportionally to this model's actual size.
    const float boundingRadius = result.document.boundingRadius();
    if (!previewRenderer_.setActiveModel(std::move(result.document))) {
        diagnostics_.addError("Imported model has no drawable meshes: " + modelPath.string());
        return false;
    }

    renderSession_.renderTargetKind = RenderTargetKind::Model;
    renderSession_.loadedModelId = modelPath.stem().string();
    // Default to playing the first animation clip (if any) so freshly imported animated models
    // behave like before this feature existed; the user can switch clips via selectAnimation().
    renderSession_.selectedAnimationIndex = previewRenderer_.activeModelAnimationNames().empty() ? -1 : 0;
    // Rescale zoom/orbit/pan sensitivity to this model's size (built-in primitives are ~1 unit
    // across, so a radius near zero would otherwise leave the camera clipped through/miles away
    // from an arbitrarily large or small imported model). setSceneScale() also resets orbit/pan.
    renderSession_.interactionState.setSceneScale(boundingRadius > 0.0F ? boundingRadius : 1.0F);
    diagnostics_.addInfo("Imported model: " + modelPath.string());
    // Model uniforms (Mat_*/gBones/textures) are bound per-mesh by PreviewRenderer, but the
    // uniform panel still needs to know about any *shader* uniforms (e.g. MVP, custom ones);
    // updateShaders() re-runs introspection against the currently active shader pair.
    return updateShaders();
}

bool WorkspaceController::selectModel() {
    if (!previewRenderer_.hasLoadedModel()) {
        return false;
    }
    renderSession_.renderTargetKind = RenderTargetKind::Model;
    // Re-apply the cached model's scale after a primitive selection restored the primitive
    // defaults. This keeps the original AABB-based framing when switching back without loading
    // the model again.
    const float boundingRadius = previewRenderer_.activeModelBoundingRadius();
    renderSession_.interactionState.setSceneScale(boundingRadius > 0.0F ? boundingRadius : 1.0F);
    return updateShaders();
}


void WorkspaceController::applyUniform(const std::string& name, UniformValue value) {
    // Uniform edits reuse the preview refresh path so the rendered result updates immediately.
    uniformState_.apply(name, std::move(value));
    updateShaders();
}

void WorkspaceController::orbitPreview(const glm::vec2& delta) { renderSession_.interactionState.orbit(delta); }

void WorkspaceController::panPreview(const glm::vec2& delta) { renderSession_.interactionState.pan(delta); }

void WorkspaceController::zoomPreview(float wheelDelta) { renderSession_.interactionState.zoom(wheelDelta); }

void WorkspaceController::resetPreviewInteraction() { renderSession_.interactionState.reset(); }

void WorkspaceController::refreshUniforms() {
    uniformState_.setDefinitions(introspectionService_.discover(editorState_.document()));
    diagnostics_.addInfo("Discovered " + std::to_string(uniformState_.definitions().size()) + " shader uniforms.");
}
}  // namespace shadereditor
