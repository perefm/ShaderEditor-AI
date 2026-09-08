#include "rendering/opengl/PreviewRenderer.h"

#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <functional>
#include <numeric>
#include <regex>
#include <string_view>
#include <utility>

namespace shadereditor {
namespace {
std::string formatShaderError(
    const std::string& log,
    const char* stageName,
    const std::vector<ShaderStageLine>&) {
    std::smatch match;
    const std::regex linePattern(R"((?:ERROR:\s*\d+:|0\(|\(|:)\s*(\d+))");
    if (!std::regex_search(log, match, linePattern)) {
        return std::string(stageName) + " shader: " + log;
    }

    return std::string(stageName) + " shader, line " + match[1].str() + ": " + log;
}

GLuint compileShader(
    GLenum type,
    const char* source,
    const char* stageName,
    const std::vector<ShaderStageLine>& lineMap,
    std::string& errorMessage) {
    const GLuint shader = glCreateShader(type);
    std::string sourceWithDocumentLines = source;
    const auto firstNewline = sourceWithDocumentLines.find('\n');
    if (firstNewline != std::string::npos && !lineMap.empty()) {
        sourceWithDocumentLines.insert(
            firstNewline + 1,
            "#line " + std::to_string(lineMap.front().documentLine + 1) + "\n");
    }
    const char* sourceText = sourceWithDocumentLines.c_str();
    glShaderSource(shader, 1, &sourceText, nullptr);
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
    errorMessage = formatShaderError(log, stageName, lineMap);
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

    if (session.renderTargetKind == RenderTargetKind::Model) {
        // The model render path does not depend on the built-in primitive library at all;
        // it draws whatever mesh data setActiveModel() cached.
        if (activeModel_ == nullptr) {
            session.frameStatus = FrameStatus::Error;
            session.errorMessage = "No model has been loaded to render.";
            return session;
        }
        if (program_ == 0 ||
            !ensureFramebuffer(width, height, session.errorMessage) ||
            !ensureModelGpuResources(session.errorMessage)) {
            session.programStatus = ProgramStatus::Failed;
            session.frameStatus = FrameStatus::Error;
            return session;
        }

        beginModelFrame(program_, width, height, session.backgroundColor);
        applyUniforms(program_, session, uniforms);
        // The animator recomputes bone transforms fresh every frame from the current playback
        // time, so animation always reflects Play/Pause/Reset state exactly (no stale caching).
        const std::vector<glm::mat4> boneTransforms =
            skeletalAnimator_.boneTransforms(*activeModel_, session.playback.elapsedSeconds(), session.selectedAnimationIndex, session.animationLoopMode);
        // gBones is identical for every mesh of the model, so it is uploaded once per frame rather
        // than once per mesh (FR-014).
        if (!boneTransforms.empty()) {
            const GLint bonesLocation = glGetUniformLocation(program_, "gBones");
            if (bonesLocation >= 0) {
                glUniformMatrix4fv(bonesLocation, static_cast<GLsizei>(boneTransforms.size()), GL_FALSE,
                                   glm::value_ptr(boneTransforms.front()));
            }
        }

        const ResolvedCamera camera = resolveActiveCamera(session);
        const glm::mat4 baseModelMatrix = previewCamera_.modelMatrix(session.interactionState);

        // The whole node hierarchy is evaluated once per frame rather than once per mesh. Walking
        // it per mesh is O(meshes * nodes) and allocates each time, which dominated frame time on
        // scenes made of many small meshes (planets.fbx: 447 meshes over 255 nodes).
        const bool useNodeTransforms = !activeModel_->hasSkeleton && !activeModel_->sceneNodes.empty();
        const std::vector<glm::mat4> nodeTransforms =
            useNodeTransforms ? skeletalAnimator_.nodeWorldTransforms(*activeModel_, session.playback.elapsedSeconds(),
                                                                     session.selectedAnimationIndex, session.animationLoopMode)
                              : std::vector<glm::mat4>();

        // Draw meshes grouped by material so identical material uniforms and textures are bound
        // once per material instead of once per mesh. Entries are instances, not meshes: a mesh
        // referenced by many nodes must be drawn once per placement.
        const std::vector<std::size_t>& drawOrder = materialSortedMeshOrder();
        int boundMaterial = -1;
        GLuint boundVao = 0;
        for (const std::size_t instanceIndex : drawOrder) {
            const ModelMeshInstance& instance = activeModel_->meshInstances[instanceIndex];
            ModelMesh& mesh = activeModel_->meshes[instance.meshIndex];

            // Node-level keyframe animation (an object that moves without a skeleton) lives in the
            // mesh's scene-node transform, so it must be folded into the model matrix per instance.
            // Skinned models are deliberately excluded: gBones already bakes the node hierarchy in,
            // and applying it again here would transform those vertices twice.
            glm::mat4 meshModelMatrix = baseModelMatrix;
            if (useNodeTransforms && instance.nodeIndex >= 0 &&
                static_cast<std::size_t>(instance.nodeIndex) < nodeTransforms.size()) {
                meshModelMatrix = baseModelMatrix * nodeTransforms[static_cast<std::size_t>(instance.nodeIndex)];
            }
            uploadMeshTransforms(program_, camera, meshModelMatrix);

            if (mesh.materialIndex < 0 || mesh.materialIndex != boundMaterial) {
                bindMeshMaterial(mesh.material, program_);
                boundMaterial = mesh.materialIndex;
            }

            const ModelMeshBuffers& buffers = activeModelBuffers_[instance.meshIndex];
            if (buffers.vao != boundVao) {
                glBindVertexArray(buffers.vao);
                boundVao = buffers.vao;
            }
            glDrawElements(GL_TRIANGLES, buffers.indexCount, GL_UNSIGNED_INT, nullptr);
        }

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

    const auto* primitive = library_.findById(session.selectedPrimitiveId);
    if (primitive == nullptr) {
        session.frameStatus = FrameStatus::Error;
        session.errorMessage = "Unknown preview primitive: " + session.selectedPrimitiveId;
        return session;
    }

    if (program_ == 0 ||
        !ensureFramebuffer(width, height, session.errorMessage) ||
        !ensureMesh(*primitive, session.errorMessage)) {
        session.programStatus = ProgramStatus::Failed;
        session.frameStatus = FrameStatus::Error;
        return session;
    }

    renderPrimitive(*primitive, program_, width, height, session.backgroundColor);
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
        if (mesh.uvbo != 0) {
            glDeleteBuffers(1, &mesh.uvbo);
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
    for (const auto& [_, texture] : textures_) {
        if (texture != 0) {
            glDeleteTextures(1, &texture);
        }
    }
    textures_.clear();

    if (colorTexture_ != 0) {
        glDeleteTextures(1, &colorTexture_);
        colorTexture_ = 0;
    }
    if (framebuffer_ != 0) {
        glDeleteFramebuffers(1, &framebuffer_);
        framebuffer_ = 0;
    }

    for (auto& buffers : activeModelBuffers_) {
        if (buffers.indexBuffer != 0) {
            glDeleteBuffers(1, &buffers.indexBuffer);
        }
        if (buffers.vertexBuffer != 0) {
            glDeleteBuffers(1, &buffers.vertexBuffer);
        }
        if (buffers.vao != 0) {
            glDeleteVertexArrays(1, &buffers.vao);
        }
    }
    activeModelBuffers_.clear();
    materialSortedOrder_.clear();
    activeModelGpuResourcesReady_ = false;
}

bool PreviewRenderer::hasOpenGlContext() const { return glfwGetCurrentContext() != nullptr; }

bool PreviewRenderer::ensureProgram(const ShaderPairDocument& document, std::string& errorMessage) {
    if (program_ != 0 && compiledVertexSource_ == document.vertexSource && compiledFragmentSource_ == document.fragmentSource) {
        return true;
    }

    const GLuint vertexShader = compileShader(
        GL_VERTEX_SHADER, document.vertexSource.c_str(), "Vertex", document.vertexLineMap, errorMessage);
    if (vertexShader == 0) {
        return false;
    }

    const GLuint fragmentShader = compileShader(
        GL_FRAGMENT_SHADER, document.fragmentSource.c_str(), "Fragment", document.fragmentLineMap, errorMessage);
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
    if (primitive.texcoords.size() != primitive.vertices.size()) {
        errorMessage = "Preview primitive has invalid UV coordinates: " + primitive.id;
        return false;
    }

    MeshBuffers mesh;
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.uvbo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(primitive.vertices.size() * sizeof(glm::vec3)), primitive.vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.uvbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(primitive.texcoords.size() * sizeof(glm::vec2)),
        primitive.texcoords.data(),
        GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    mesh.vertexCount = static_cast<GLsizei>(primitive.vertices.size());
    meshes_.emplace(primitive.id, mesh);
    return true;
}

std::vector<std::string> PreviewRenderer::activeModelAnimationNames() const {
    std::vector<std::string> names;
    if (activeModel_ == nullptr) {
        return names;
    }
    names.reserve(activeModel_->animations.size());
    for (const auto& clip : activeModel_->animations) {
        // Some formats (e.g. FBX) leave clip names blank; fall back to an index-based label so
        // every entry is still selectable and distinguishable in the UI.
        names.push_back(clip.name.empty() ? ("Animation " + std::to_string(names.size())) : clip.name);
    }
    return names;
}

std::vector<std::string> PreviewRenderer::activeModelCameraNames() const {
    std::vector<std::string> names;
    if (activeModel_ == nullptr) {
        return names;
    }
    names.reserve(activeModel_->cameras.size());
    for (const auto& camera : activeModel_->cameras) {
        names.push_back(camera.name.empty() ? ("Camera " + std::to_string(names.size())) : camera.name);
    }
    return names;
}

ResolvedCamera PreviewRenderer::resolveActiveCamera(const RenderSession& session) const {
    const float aspect = framebufferHeight_ > 0 ? static_cast<float>(framebufferWidth_) / static_cast<float>(framebufferHeight_) : 1.0F;

    if (session.renderTargetKind == RenderTargetKind::Model && activeModel_ != nullptr && session.activeCameraIndex >= 0) {
        // Scene cameras use the same near/far scaling rule as the free camera so a model camera
        // never clips geometry the free camera would have shown.
        const float nearPlane = 0.05F * session.interactionState.sceneScale;
        const float farPlane = 200.0F * session.interactionState.sceneScale;
        const auto resolved = modelCameraResolver_.resolve(*activeModel_,
                                                           session.activeCameraIndex,
                                                           session.playback.elapsedSeconds(),
                                                           session.selectedAnimationIndex,
                                                           session.animationLoopMode,
                                                           aspect,
                                                           glm::radians(45.0F),
                                                           nearPlane,
                                                           farPlane);
        if (resolved.has_value()) {
            return *resolved;
        }
        // An invalid index falls through to the free camera rather than failing to render.
    }

    ResolvedCamera freeCamera;
    freeCamera.view = previewCamera_.view(session.interactionState);
    freeCamera.projection = previewCamera_.projection(session.interactionState, aspect);
    freeCamera.position = previewCamera_.position(session.interactionState);
    return freeCamera;
}

bool PreviewRenderer::setActiveModel(ModelDocument document) {
    if (document.meshes.empty()) {
        return false;
    }
    // Any previously loaded model's GPU buffers are stale once the CPU-side document changes;
    // drop them so ensureModelGpuResources() rebuilds everything for the new model next frame.
    for (auto& buffers : activeModelBuffers_) {
        if (buffers.indexBuffer != 0) {
            glDeleteBuffers(1, &buffers.indexBuffer);
        }
        if (buffers.vertexBuffer != 0) {
            glDeleteBuffers(1, &buffers.vertexBuffer);
        }
        if (buffers.vao != 0) {
            glDeleteVertexArrays(1, &buffers.vao);
        }
    }
    activeModelBuffers_.clear();
    materialSortedOrder_.clear();
    activeModelGpuResourcesReady_ = false;
    activeModel_ = std::make_unique<ModelDocument>(std::move(document));
    return true;
}

bool PreviewRenderer::ensureModelGpuResources(std::string& errorMessage) {
    if (activeModelGpuResourcesReady_ || activeModel_ == nullptr) {
        return activeModel_ != nullptr;
    }
    if (!hasOpenGlContext()) {
        errorMessage = "No OpenGL context is available to upload the imported model.";
        return false;
    }

    activeModelBuffers_.resize(activeModel_->meshes.size());
    for (std::size_t meshIndex = 0; meshIndex < activeModel_->meshes.size(); ++meshIndex) {
        const ModelMesh& mesh = activeModel_->meshes[meshIndex];
        ModelMeshBuffers& buffers = activeModelBuffers_[meshIndex];

        glGenVertexArrays(1, &buffers.vao);
        glGenBuffers(1, &buffers.vertexBuffer);
        glGenBuffers(1, &buffers.indexBuffer);

        glBindVertexArray(buffers.vao);
        glBindBuffer(GL_ARRAY_BUFFER, buffers.vertexBuffer);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(ModelVertex)),
            mesh.vertices.data(),
            GL_STATIC_DRAW);

        // Attribute layout matches Phoenix's Mesh::setupMesh exactly (see ModelDocument.h):
        // location 0 = aPos, 1 = aNormal, 2 = aTexCoords, 3 = aTangent, 4 = aBiTangent,
        // 5 = aBoneID (integer attribute), 6 = aBoneWeight.
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), reinterpret_cast<void*>(offsetof(ModelVertex, position)));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), reinterpret_cast<void*>(offsetof(ModelVertex, normal)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), reinterpret_cast<void*>(offsetof(ModelVertex, texCoords)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), reinterpret_cast<void*>(offsetof(ModelVertex, tangent)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), reinterpret_cast<void*>(offsetof(ModelVertex, biTangent)));
        glEnableVertexAttribArray(4);
        glVertexAttribIPointer(5, 4, GL_UNSIGNED_INT, sizeof(ModelVertex), reinterpret_cast<void*>(offsetof(ModelVertex, boneIds)));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), reinterpret_cast<void*>(offsetof(ModelVertex, boneWeights)));
        glEnableVertexAttribArray(6);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.indexBuffer);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(unsigned int)),
            mesh.indices.data(),
            GL_STATIC_DRAW);
        buffers.indexCount = static_cast<GLsizei>(mesh.indices.size());

        glBindVertexArray(0);
    }

    activeModelGpuResourcesReady_ = true;
    return true;
}

GLuint PreviewRenderer::textureForPath(const std::filesystem::path& path) {
    const std::string key = path.string();
    const auto cached = textures_.find(key);
    if (cached != textures_.end()) {
        return cached->second;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    // glTF/Assimp UV coordinates use a top-left image origin, while OpenGL samples texture
    // rows from the bottom. Flip decoded image data once so model UVs remain unchanged and
    // external and embedded textures use the same convention.
    stbi_set_flip_vertically_on_load(1);
    stbi_uc* pixels = stbi_load(key.c_str(), &width, &height, &channels, 4);
    if (pixels == nullptr) {
        return 0;
    }

    const GLuint texture = uploadRgbaTexture(pixels, width, height);
    stbi_image_free(pixels);
    textures_.emplace(key, texture);
    return texture;
}

GLuint PreviewRenderer::textureForEmbeddedData(const std::vector<unsigned char>& encodedBytes) {
    // Embedded (glTF/.glb) textures have no file path, so cache them by a hash of their encoded
    // bytes instead - cheap to compute once per mesh load and stable across frames.
    const std::size_t key = std::hash<std::string_view>{}(
        std::string_view(reinterpret_cast<const char*>(encodedBytes.data()), encodedBytes.size()));
    const std::string cacheKey = "embedded:" + std::to_string(key);
    const auto cached = textures_.find(cacheKey);
    if (cached != textures_.end()) {
        return cached->second;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(1);
    stbi_uc* pixels = stbi_load_from_memory(encodedBytes.data(), static_cast<int>(encodedBytes.size()), &width, &height, &channels, 4);
    if (pixels == nullptr) {
        return 0;
    }

    const GLuint texture = uploadRgbaTexture(pixels, width, height);
    stbi_image_free(pixels);
    textures_.emplace(cacheKey, texture);
    return texture;
}

GLuint PreviewRenderer::uploadRgbaTexture(const unsigned char* pixels, int width, int height) {
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    return texture;
}

void PreviewRenderer::applyUniforms(GLuint program, const RenderSession& session, const std::vector<UniformDefinition>& uniforms) {
    glUseProgram(program);

    // One resolved camera drives every camera-derived uniform this frame. Because "MVP" is
    // literally projection * view * model, it can never disagree with the individual matrices,
    // and uCameraPos always follows the currently selected camera (free or scene, static or
    // animated) without any cross-frame caching to invalidate.
    const ResolvedCamera camera = resolveActiveCamera(session);
    const glm::mat4 modelMatrix = previewCamera_.modelMatrix(session.interactionState);
    uploadMeshTransforms(program, camera, modelMatrix);

    int textureUnit = 0;
    for (const auto& uniform : uniforms) {
        const GLint location = glGetUniformLocation(program, uniform.name.c_str());
        if (location < 0 || uniform.name == "MVP" || uniform.name == "uCameraPos" || uniform.name == "model" ||
            uniform.name == "view" || uniform.name == "projection") {
            continue;
        }

        // Phoenix auto-uniforms are driven live by the playback clock every frame, overriding
        // whatever stale value might be cached in RenderSession::uniformValues for that name
        // (FR-009/FR-010: the app - not the user - controls t/tend/beat).
        if (uniform.provenance == UniformProvenance::PhoenixAuto) {
            if (uniform.name == "t") {
                glUniform1f(location, session.playback.elapsedSeconds());
            } else if (uniform.name == "tend") {
                glUniform1f(location, session.playback.sectionDurationSeconds());
            } else if (uniform.name == "beat") {
                glUniform1f(location, session.playback.beat());
            } else if (uniform.name == "vpWidth") {
                glUniform1f(location, static_cast<float>(session.viewport.widthPixels));
            } else if (uniform.name == "vpHeight") {
                glUniform1f(location, static_cast<float>(session.viewport.heightPixels));
            } else if (uniform.name == "aspectRatio") {
                glUniform1f(location, session.viewport.aspectRatio);
            }
            // Mat_Ka/Mat_Kd/Mat_Ks/Mat_KsStrenght/gBones are uploaded separately, per active mesh,
            // by the model-rendering path (see renderModel/applyMaterialUniforms) since their
            // values depend on which mesh is currently bound, not on global playback state.
            continue;
        }

        // Upload the exact runtime shape discovered by the uniform metadata layer.
        if (const auto* value = std::get_if<std::string>(&uniform.currentValue)) {
            if (value->empty()) {
                continue;
            }
            GLuint texture = 0;
            const auto cached = textures_.find(*value);
            if (cached != textures_.end()) {
                texture = cached->second;
            } else {
                int width = 0;
                int height = 0;
                int channels = 0;
                stbi_uc* pixels = stbi_load(value->c_str(), &width, &height, &channels, 4);
                if (pixels == nullptr) {
                    continue;
                }
                glGenTextures(1, &texture);
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                stbi_image_free(pixels);
                textures_.emplace(*value, texture);
            }
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, texture);
            glUniform1i(location, textureUnit);
            ++textureUnit;
            continue;
        }

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

void PreviewRenderer::renderPrimitive(const PreviewPrimitive& primitive, GLuint program, int width, int height, const glm::vec4& clearColor) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    // Clear the offscreen target every frame so the ImGui panel always shows a complete preview image.
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);
    glBindVertexArray(meshes_.at(primitive.id).vao);
}

void PreviewRenderer::beginModelFrame(GLuint program, int width, int height, const glm::vec4& clearColor) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);
}

void PreviewRenderer::uploadMeshTransforms(GLuint program, const ResolvedCamera& camera, const glm::mat4& modelMatrix) {
    const GLint viewLocation = glGetUniformLocation(program, "view");
    if (viewLocation >= 0) {
        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(camera.view));
    }
    const GLint projectionLocation = glGetUniformLocation(program, "projection");
    if (projectionLocation >= 0) {
        glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(camera.projection));
    }
    const GLint mvpLocation = glGetUniformLocation(program, "MVP");
    if (mvpLocation >= 0) {
        const glm::mat4 mvp = camera.projection * camera.view * modelMatrix;
        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));
    }
    const GLint cameraLocation = glGetUniformLocation(program, "uCameraPos");
    if (cameraLocation >= 0) {
        glUniform3fv(cameraLocation, 1, glm::value_ptr(camera.position));
    }
    const GLint modelLocation = glGetUniformLocation(program, "model");
    if (modelLocation >= 0) {
        // Phoenix shaders (bone_animation/bump_mapping/pbr_animation, etc.) declare a "model"
        // uniform for transforming normals/tangents into world space; like MVP/view/projection/
        // uCameraPos this is entirely engine-owned, never user-edited (see FR-012).
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    }
}

void PreviewRenderer::bindMeshMaterial(ModelMaterial& material, GLuint program) {
    // Upload the material colors using Phoenix's exact uniform names (FR-016) so bump-mapping,
    // PBR, etc. shaders authored against Phoenix conventions work unmodified against imports.
    const GLint ambientLocation = glGetUniformLocation(program, "Mat_Ka");
    if (ambientLocation >= 0) {
        glUniform3fv(ambientLocation, 1, glm::value_ptr(material.colorAmbient));
    }
    const GLint diffuseLocation = glGetUniformLocation(program, "Mat_Kd");
    if (diffuseLocation >= 0) {
        glUniform3fv(diffuseLocation, 1, glm::value_ptr(material.colorDiffuse));
    }
    const GLint specularLocation = glGetUniformLocation(program, "Mat_Ks");
    if (specularLocation >= 0) {
        glUniform3fv(specularLocation, 1, glm::value_ptr(material.colorSpecular));
    }
    const GLint specularStrengthLocation = glGetUniformLocation(program, "Mat_KsStrenght");
    if (specularStrengthLocation >= 0) {
        glUniform1f(specularStrengthLocation, material.specularStrength);
    }

    // Bind every texture slot the material carries, using the exact Phoenix uniform name
    // (e.g. "texture_diffuse1") that AssimpModelLoader assigned for each (see FR-015). Textures
    // may either live on disk (sourcePath) or be embedded directly in the model file (glTF/.glb),
    // in which case embeddedImageData holds the still-encoded bytes to decode instead.
    int textureUnit = 0;
    for (auto& slot : material.textureSlots) {
        const GLint location = glGetUniformLocation(program, slot.shaderUniformName.c_str());
        if (location < 0) {
            continue;
        }
        // ModelTextureSlot::glTextureId memoizes the resolved GL texture for this slot. Without it
        // an embedded texture would be re-hashed over its full encoded byte range on every bind,
        // every frame, just to look up an already-uploaded texture.
        GLuint texture = slot.glTextureId;
        if (texture == 0) {
            texture = slot.embeddedImageData.empty() ? textureForPath(slot.sourcePath)
                                                     : textureForEmbeddedData(slot.embeddedImageData);
            slot.glTextureId = texture;
        }
        if (texture == 0) {
            continue;
        }
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(location, textureUnit);
        ++textureUnit;
    }
}

const std::vector<std::size_t>& PreviewRenderer::materialSortedMeshOrder() {
    // Computed once per loaded model, not per frame: the material assignment is fixed for the
    // lifetime of the document, so the draw order never changes. Entries index meshInstances, so
    // every placement of an instanced mesh gets drawn.
    if (!materialSortedOrder_.empty() || activeModel_ == nullptr || activeModel_->meshInstances.empty()) {
        return materialSortedOrder_;
    }
    materialSortedOrder_.resize(activeModel_->meshInstances.size());
    std::iota(materialSortedOrder_.begin(), materialSortedOrder_.end(), std::size_t {0});
    // Stable sort keeps the model's authored order within a material group, so draw order stays
    // deterministic (important for meshes that rely on back-to-front blending order).
    std::stable_sort(materialSortedOrder_.begin(), materialSortedOrder_.end(),
                     [this](std::size_t lhs, std::size_t rhs) {
                         const auto& meshes = activeModel_->meshes;
                         const auto& instances = activeModel_->meshInstances;
                         return meshes[instances[lhs].meshIndex].materialIndex <
                                meshes[instances[rhs].meshIndex].materialIndex;
                     });
    return materialSortedOrder_;
}
}  // namespace shadereditor
