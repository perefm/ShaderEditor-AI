#include "app/workspace/WorkspaceController.h"

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

bool WorkspaceController::updateShaders() {
    // Compile first so a failed attempt cannot replace the last valid uniform state.
    const auto previousInteraction = renderSession_.interactionState;
    const auto previousPrimitive = renderSession_.selectedPrimitiveId;
    const auto nextSession = previewRenderer_.updatePreview(editorState_.document(), previousPrimitive, uniformState_.definitions());
    if (nextSession.frameStatus == FrameStatus::Error) {
        renderSession_.errorMessage = nextSession.errorMessage;
        renderSession_.programStatus = nextSession.programStatus;
        renderSession_.frameStatus = nextSession.frameStatus;
        diagnostics_.addError(renderSession_.errorMessage);
        return false;
    }

    renderSession_ = nextSession;
    renderSession_.interactionState = previousInteraction;
    refreshUniforms();
    diagnostics_.addInfo("Updated shader preview.");
    return true;
}

const RenderSession& WorkspaceController::renderPreview(int width, int height) {
    // Rendering is lazy so frames that only rearrange UI do not rebuild the preview texture needlessly.
    renderSession_ = previewRenderer_.renderFrame(editorState_.document(), renderSession_, uniformState_.definitions(), width, height);
    return renderSession_;
}

bool WorkspaceController::handleKeyChord(const std::string& chord) { return chord == "Ctrl+Enter" ? updateShaders() : false; }

void WorkspaceController::selectPrimitive(const std::string& primitiveId) {
    // Primitive changes go through the same update path as shader edits to keep diagnostics unified.
    renderSession_.selectedPrimitiveId = primitiveId;
    updateShaders();
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
