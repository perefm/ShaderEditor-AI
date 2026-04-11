#include "app/workspace/WorkspaceController.h"

namespace shadereditor {
WorkspaceController::WorkspaceController(DiagnosticsState& diagnostics) : diagnostics_(diagnostics) {}

bool WorkspaceController::openShaders(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) {
    try {
        // Loading the shader pair together gives the preview everything it needs in one pass.
        editorState_.attachDocument(fileService_.load(vertexPath, fragmentPath));
        diagnostics_.addInfo("Loaded shader pair.");
        refreshUniforms();
        return updateShaders();
    } catch (const std::exception& ex) {
        diagnostics_.addError(ex.what());
        return false;
    }
}

bool WorkspaceController::openVertexShader(const std::filesystem::path& vertexPath) {
    try {
        auto& document = editorState_.document();
        document.vertexPath = vertexPath;
        document.vertexSource = fileService_.loadSource(vertexPath);
        document.markLoaded();
        diagnostics_.addInfo("Loaded vertex shader.");
        refreshUniforms();
        if (!document.fragmentSource.empty()) {
            return updateShaders();
        }
        // Keep the preview idle until the complementary stage is available.
        renderSession_.frameStatus = FrameStatus::Idle;
        renderSession_.previewSummary = "Vertex shader loaded. Load a fragment shader to render.";
        renderSession_.errorMessage.clear();
        return true;
    } catch (const std::exception& ex) {
        diagnostics_.addError(ex.what());
        return false;
    }
}

bool WorkspaceController::openFragmentShader(const std::filesystem::path& fragmentPath) {
    try {
        auto& document = editorState_.document();
        document.fragmentPath = fragmentPath;
        document.fragmentSource = fileService_.loadSource(fragmentPath);
        document.markLoaded();
        diagnostics_.addInfo("Loaded fragment shader.");
        refreshUniforms();
        if (!document.vertexSource.empty()) {
            return updateShaders();
        }
        // Keep the preview idle until the complementary stage is available.
        renderSession_.frameStatus = FrameStatus::Idle;
        renderSession_.previewSummary = "Fragment shader loaded. Load a vertex shader to render.";
        renderSession_.errorMessage.clear();
        return true;
    } catch (const std::exception& ex) {
        diagnostics_.addError(ex.what());
        return false;
    }
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
    // Rebuild the logical preview state first; actual framebuffer rendering happens lazily in the render panel.
    renderSession_ = previewRenderer_.updatePreview(editorState_.document(), renderSession_.selectedPrimitiveId, uniformState_.definitions());
    if (renderSession_.frameStatus == FrameStatus::Error) {
        diagnostics_.addError(renderSession_.errorMessage);
        return false;
    }
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

void WorkspaceController::resetPreviewInteraction() { renderSession_.interactionState.reset(); }

void WorkspaceController::refreshUniforms() {
    uniformState_.setDefinitions(introspectionService_.discover(editorState_.document()));
    diagnostics_.addInfo("Discovered " + std::to_string(uniformState_.definitions().size()) + " shader uniforms.");
}
}  // namespace shadereditor
