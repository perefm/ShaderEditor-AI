#pragma once

#include "editor/ShaderPairDocument.h"
#include "rendering/geometry/PrimitiveLibrary.h"
#include "rendering/opengl/PreviewCamera.h"
#include "rendering/shaders/RenderSession.h"
#include "rendering/shaders/ShaderProgramService.h"
#include "rendering/shaders/UniformDefinition.h"

#include <glad/glad.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace shadereditor {
class PreviewRenderer {
  public:
    PreviewRenderer() = default;
    ~PreviewRenderer();

    RenderSession updatePreview(const ShaderPairDocument& document,
                                const std::string& primitiveId,
                                const std::vector<UniformDefinition>& uniforms);
    RenderSession renderFrame(const ShaderPairDocument& document,
                              RenderSession session,
                              const std::vector<UniformDefinition>& uniforms,
                              int width,
                              int height);

  private:
    struct MeshBuffers {
        GLuint vao {0};
        GLuint vbo {0};
        GLsizei vertexCount {0};
    };

    void destroyGpuResources();
    bool hasOpenGlContext() const;
    bool ensureProgram(const ShaderPairDocument& document, std::string& errorMessage);
    bool ensureFramebuffer(int width, int height, std::string& errorMessage);
    bool ensureMesh(const PreviewPrimitive& primitive, std::string& errorMessage);
    void applyUniforms(GLuint program, const RenderSession& session, const std::vector<UniformDefinition>& uniforms) const;
    void renderPrimitive(const PreviewPrimitive& primitive, GLuint program, int width, int height);

    PreviewCamera previewCamera_;
    PrimitiveLibrary library_;
    ShaderProgramService shaderProgramService_;
    GLuint program_ {0};
    GLuint framebuffer_ {0};
    GLuint colorTexture_ {0};
    GLuint depthStencilBuffer_ {0};
    int framebufferWidth_ {0};
    int framebufferHeight_ {0};
    std::string compiledVertexSource_;
    std::string compiledFragmentSource_;
    std::unordered_map<std::string, MeshBuffers> meshes_;
};
}  // namespace shadereditor
