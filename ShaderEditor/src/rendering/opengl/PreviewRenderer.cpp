#include "rendering/opengl/PreviewRenderer.h"

#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>

#include <utility>

namespace shadereditor {
namespace {
GLuint compileShader(GLenum type, const char* source, std::string& errorMessage) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_TRUE) {
        return shader;
    }

    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<std::size_t>(std::max(logLength, 1)), '\0');
    glGetShaderInfoLog(shader, logLength, nullptr, log.data());
    glDeleteShader(shader);
    errorMessage = std::move(log);
    return 0;
}
}

PreviewRenderer::~PreviewRenderer() { destroyGpuResources(); }

RenderSession PreviewRenderer::updatePreview(const ShaderPairDocument& document,
                                             const std::string& primitiveId,
                                             const std::vector<UniformDefinition>& uniforms) {
    RenderSession session;
    session.selectedPrimitiveId = primitiveId;

    const auto* primitive = library_.findById(primitiveId);
    if (primitive == nullptr) {
        session.programStatus = ProgramStatus::Failed;
        session.frameStatus = FrameStatus::Error;
        session.errorMessage = "Unknown preview primitive: " + primitiveId;
        return session;
    }

    // Keep the high-level validation path separate from GPU resource creation so unit tests can call it safely.
    const auto compile = shaderProgramService_.compileAndLink(document);
    if (!compile.success) {
        session.programStatus = ProgramStatus::Failed;
        session.frameStatus = FrameStatus::Error;
        session.errorMessage = compile.errorMessage;
        return session;
    }

    session.programStatus = ProgramStatus::Linked;
    session.frameStatus = FrameStatus::Rendering;
    session.previewSummary = "Rendered " + primitive->label;
    for (const auto& uniform : uniforms) {
        session.uniformValues[uniform.name] = uniform.currentValue;
    }

    if (!hasOpenGlContext()) {
        return session;
    }

    std::string gpuError;
    if (!ensureProgram(document, gpuError)) {
        session.programStatus = ProgramStatus::Failed;
        session.frameStatus = FrameStatus::Error;
        session.errorMessage = gpuError;
    }
    return session;
}

RenderSession PreviewRenderer::renderFrame(const ShaderPairDocument& document,
                                           RenderSession session,
                                           const std::vector<UniformDefinition>& uniforms,
                                           int width,
                                           int height) {
    if (!hasOpenGlContext() || width <= 0 || height <= 0) {
        return session;
    }

    if (session.programStatus == ProgramStatus::Failed) {
        return session;
    }

    const auto* primitive = library_.findById(session.selectedPrimitiveId);
    if (primitive == nullptr) {
        session.frameStatus = FrameStatus::Error;
        session.errorMessage = "Unknown preview primitive: " + session.selectedPrimitiveId;
        return session;
    }

    if (!ensureProgram(document, session.errorMessage) ||
        !ensureFramebuffer(width, height, session.errorMessage) ||
        !ensureMesh(*primitive, session.errorMessage)) {
        session.programStatus = ProgramStatus::Failed;
        session.frameStatus = FrameStatus::Error;
        return session;
    }

    renderPrimitive(*primitive, program_, width, height);
    applyUniforms(program_, session, uniforms);
    glDrawArrays(GL_TRIANGLES, 0, meshes_.at(primitive->id).vertexCount);

    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    session.previewTextureId = colorTexture_;
    session.previewWidth = width;
    session.previewHeight = height;
    session.frameStatus = FrameStatus::Rendering;
    session.errorMessage.clear();
    return session;
}

void PreviewRenderer::destroyGpuResources() {
    for (auto& [_, mesh] : meshes_) {
        if (mesh.vbo != 0) {
            glDeleteBuffers(1, &mesh.vbo);
        }
        if (mesh.vao != 0) {
            glDeleteVertexArrays(1, &mesh.vao);
        }
    }
    meshes_.clear();

    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    if (depthStencilBuffer_ != 0) {
        glDeleteRenderbuffers(1, &depthStencilBuffer_);
        depthStencilBuffer_ = 0;
    }
    if (colorTexture_ != 0) {
        glDeleteTextures(1, &colorTexture_);
        colorTexture_ = 0;
    }
    if (framebuffer_ != 0) {
        glDeleteFramebuffers(1, &framebuffer_);
        framebuffer_ = 0;
    }
}

bool PreviewRenderer::hasOpenGlContext() const { return glfwGetCurrentContext() != nullptr; }

bool PreviewRenderer::ensureProgram(const ShaderPairDocument& document, std::string& errorMessage) {
    if (program_ != 0 && compiledVertexSource_ == document.vertexSource && compiledFragmentSource_ == document.fragmentSource) {
        return true;
    }

    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, document.vertexSource.c_str(), errorMessage);
    if (vertexShader == 0) {
        return false;
    }

    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, document.fragmentSource.c_str(), errorMessage);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    const GLuint nextProgram = glCreateProgram();
    glAttachShader(nextProgram, vertexShader);
    glAttachShader(nextProgram, fragmentShader);
    glLinkProgram(nextProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success = GL_FALSE;
    glGetProgramiv(nextProgram, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(nextProgram, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<std::size_t>(std::max(logLength, 1)), '\0');
        glGetProgramInfoLog(nextProgram, logLength, nullptr, log.data());
        glDeleteProgram(nextProgram);
        errorMessage = std::move(log);
        return false;
    }

    if (program_ != 0) {
        glDeleteProgram(program_);
    }
    program_ = nextProgram;
    compiledVertexSource_ = document.vertexSource;
    compiledFragmentSource_ = document.fragmentSource;
    return true;
}

bool PreviewRenderer::ensureFramebuffer(int width, int height, std::string& errorMessage) {
    if (framebuffer_ != 0 && framebufferWidth_ == width && framebufferHeight_ == height) {
        return true;
    }

    if (framebuffer_ == 0) {
        glGenFramebuffers(1, &framebuffer_);
    }
    if (colorTexture_ == 0) {
        glGenTextures(1, &colorTexture_);
    }
    if (depthStencilBuffer_ == 0) {
        glGenRenderbuffers(1, &depthStencilBuffer_);
    }

    glBindTexture(GL_TEXTURE_2D, colorTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindRenderbuffer(GL_RENDERBUFFER, depthStencilBuffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture_, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilBuffer_);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        errorMessage = "Preview framebuffer is incomplete.";
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    framebufferWidth_ = width;
    framebufferHeight_ = height;
    return true;
}

bool PreviewRenderer::ensureMesh(const PreviewPrimitive& primitive, std::string& errorMessage) {
    if (meshes_.contains(primitive.id)) {
        return true;
    }

    if (primitive.vertices.empty()) {
        errorMessage = "Preview primitive has no geometry: " + primitive.id;
        return false;
    }

    MeshBuffers mesh;
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(primitive.vertices.size() * sizeof(glm::vec3)), primitive.vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    mesh.vertexCount = static_cast<GLsizei>(primitive.vertices.size());
    meshes_.emplace(primitive.id, mesh);
    return true;
}

void PreviewRenderer::applyUniforms(GLuint program, const RenderSession& session, const std::vector<UniformDefinition>& uniforms) const {
    glUseProgram(program);

    const GLint mvpLocation = glGetUniformLocation(program, "u_mvp");
    if (mvpLocation >= 0) {
        // The preview camera owns all scene navigation so shaders can rely on a consistent MVP uniform.
        const float aspect = framebufferHeight_ > 0 ? static_cast<float>(framebufferWidth_) / static_cast<float>(framebufferHeight_) : 1.0F;
        const glm::mat4 mvp = previewCamera_.viewProjection(session.interactionState, aspect);
        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));
    }

    for (const auto& uniform : uniforms) {
        const GLint location = glGetUniformLocation(program, uniform.name.c_str());
        if (location < 0 || uniform.name == "u_mvp") {
            continue;
        }

        // Upload the exact runtime shape discovered by the uniform metadata layer.
        if (const auto* value = std::get_if<bool>(&uniform.currentValue)) {
            glUniform1i(location, *value ? 1 : 0);
            continue;
        }
        if (const auto* value = std::get_if<int>(&uniform.currentValue)) {
            glUniform1i(location, *value);
            continue;
        }
        if (const auto* value = std::get_if<float>(&uniform.currentValue)) {
            glUniform1f(location, *value);
            continue;
        }
        if (const auto* value = std::get_if<glm::vec2>(&uniform.currentValue)) {
            glUniform2fv(location, 1, glm::value_ptr(*value));
            continue;
        }
        if (const auto* value = std::get_if<glm::vec3>(&uniform.currentValue)) {
            glUniform3fv(location, 1, glm::value_ptr(*value));
            continue;
        }
        if (const auto* value = std::get_if<glm::vec4>(&uniform.currentValue)) {
            glUniform4fv(location, 1, glm::value_ptr(*value));
            continue;
        }
        if (const auto* value = std::get_if<glm::mat2>(&uniform.currentValue)) {
            glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(*value));
            continue;
        }
        if (const auto* value = std::get_if<glm::mat3>(&uniform.currentValue)) {
            glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(*value));
            continue;
        }
        if (const auto* value = std::get_if<glm::mat4>(&uniform.currentValue)) {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(*value));
        }
    }
}

void PreviewRenderer::renderPrimitive(const PreviewPrimitive& primitive, GLuint program, int width, int height) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    // Clear the offscreen target every frame so the ImGui panel always shows a complete preview image.
    glClearColor(0.09F, 0.10F, 0.13F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);
    glBindVertexArray(meshes_.at(primitive.id).vao);
}
}  // namespace shadereditor
