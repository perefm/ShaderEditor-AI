#pragma once

#include "editor/ShaderPairDocument.h"
#include "rendering/geometry/PrimitiveLibrary.h"
#include "rendering/models/ModelDocument.h"
#include "rendering/models/SkeletalAnimator.h"
#include "rendering/opengl/PreviewCamera.h"
#include "rendering/shaders/RenderSession.h"
#include "rendering/shaders/ShaderProgramService.h"
#include "rendering/shaders/UniformDefinition.h"

#include <glad/glad.h>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace shadereditor {
// Owns the OpenGL objects required to validate shaders and render the offscreen preview.
class PreviewRenderer {
  public:
    PreviewRenderer() = default;
    ~PreviewRenderer();

    // Validates shaders and refreshes CPU-side preview state.
    RenderSession updatePreview(const ShaderPairDocument& document,
                                const std::string& primitiveId,
                                const std::vector<UniformDefinition>& uniforms);
    // Draws the current primitive (or imported model, when session.renderTargetKind == Model)
    // into the preview framebuffer.
    RenderSession renderFrame(const ShaderPairDocument& document,
                              RenderSession session,
                              const std::vector<UniformDefinition>& uniforms,
                              int width,
                              int height);

    // Makes `document` the active render target as RenderTargetKind::Model. The renderer takes
    // ownership of the CPU-side mesh data and lazily creates GPU buffers/textures for it the
    // next time renderFrame() runs. Returns false if the model has no drawable meshes.
    bool setActiveModel(ModelDocument document);
    // True once setActiveModel() has been called successfully at least once, so
    // WorkspaceController::selectModel() can re-activate the cached model without re-importing.
    [[nodiscard]] bool hasLoadedModel() const { return activeModel_ != nullptr; }
    // Names of every animation clip the active model carries (empty if no model is loaded or the
    // model has no animations), in the same order as ModelDocument::animations / RenderSession's
    // selectedAnimationIndex, so panels can populate an animation picker.
    [[nodiscard]] std::vector<std::string> activeModelAnimationNames() const;
    // Returns the cached model's bind-pose bounding radius so WorkspaceController can restore
    // model-proportional camera framing after temporarily switching to a primitive.
    [[nodiscard]] float activeModelBoundingRadius() const {
        return activeModel_ == nullptr ? 0.0F : activeModel_->boundingRadius();
    }

  private:
    struct MeshBuffers {
        // GPU handles for one cached primitive mesh.
        GLuint vao {0};
        GLuint vbo {0};
        GLuint uvbo {0};
        GLsizei vertexCount {0};
    };

    // GPU handles + CPU vertex/index counts for one imported ModelMesh.
    struct ModelMeshBuffers {
        GLuint vao {0};
        GLuint vertexBuffer {0};
        GLuint indexBuffer {0};
        GLsizei indexCount {0};
    };

    void destroyGpuResources();
    bool hasOpenGlContext() const;
    bool ensureProgram(const ShaderPairDocument& document, std::string& errorMessage);
    bool ensureFramebuffer(int width, int height, std::string& errorMessage);
    bool ensureMesh(const PreviewPrimitive& primitive, std::string& errorMessage);
    bool ensureModelGpuResources(std::string& errorMessage);
    void applyUniforms(GLuint program, const RenderSession& session, const std::vector<UniformDefinition>& uniforms);
    // Binds one imported mesh's textures/material colors/bone matrices and issues its draw call.
    void renderModelMesh(const ModelMesh& mesh, const ModelMeshBuffers& buffers, GLuint program, const std::vector<glm::mat4>& boneTransforms);
    void renderPrimitive(const PreviewPrimitive& primitive, GLuint program, int width, int height);
    void beginModelFrame(GLuint program, int width, int height);
    GLuint textureForPath(const std::filesystem::path& path);
    // Decodes and uploads a texture embedded directly in the model file (glTF/.glb), caching it
    // by a hash of its encoded bytes since it has no file path to key on.
    GLuint textureForEmbeddedData(const std::vector<unsigned char>& encodedBytes);
    // Shared GL texture upload helper for both on-disk and embedded textures (both decode to a
    // tightly-packed RGBA8 pixel buffer via stb_image before reaching this point).
    GLuint uploadRgbaTexture(const unsigned char* pixels, int width, int height);

    PreviewCamera previewCamera_;
    PrimitiveLibrary library_;
    ShaderProgramService shaderProgramService_;
    SkeletalAnimator skeletalAnimator_;
    GLuint program_ {0};
    GLuint framebuffer_ {0};
    GLuint colorTexture_ {0};
    GLuint depthStencilBuffer_ {0};
    int framebufferWidth_ {0};
    int framebufferHeight_ {0};
    std::string compiledVertexSource_;
    std::string compiledFragmentSource_;
    std::unordered_map<std::string, MeshBuffers> meshes_;
    std::unordered_map<std::string, GLuint> textures_;

    // The currently loaded imported model (nullptr if none has been loaded yet) and its GPU
    // buffers, cached separately from the built-in primitive path (`meshes_`) so switching
    // between "primitive" and "model" render targets never needs to re-upload either one.
    std::unique_ptr<ModelDocument> activeModel_;
    std::vector<ModelMeshBuffers> activeModelBuffers_;
    bool activeModelGpuResourcesReady_ {false};
};
}  // namespace shadereditor
