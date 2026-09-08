#include "rendering/models/AssimpModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <string>
#include <unordered_map>

namespace shadereditor {
namespace {
// Converts an Assimp row-major 4x4 matrix into GLM's column-major representation.
glm::mat4 toGlmMat4(const aiMatrix4x4& m) {
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4);
}

glm::vec3 toGlmVec3(const aiVector3D& v) { return {v.x, v.y, v.z}; }

glm::quat toGlmQuat(const aiQuaternion& q) { return {q.w, q.x, q.y, q.z}; }

// Phoenix's Material::loadTextures assigns one shader uniform name per aiTextureType, using
// these exact (and, for "ambientoclussion", intentionally misspelled) strings. Reproduced
// verbatim here so real Phoenix shaders sample the correct uniform names unmodified (FR-015).
// Reference: Spontz/Phoenix @ 75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d, Material.cpp.
const char* phoenixTextureTypeName(aiTextureType type) {
    switch (type) {
        case aiTextureType_DIFFUSE: return "diffuse";
        case aiTextureType_SPECULAR: return "specular";
        case aiTextureType_AMBIENT: return "ambient";
        case aiTextureType_HEIGHT: return "height";
        case aiTextureType_NORMALS: return "normal";
        case aiTextureType_EMISSIVE: return "emissive";
        case aiTextureType_SHININESS: return "shininess";
        case aiTextureType_AMBIENT_OCCLUSION: return "ambientoclussion";
        case aiTextureType_METALNESS: return "metalness";
        case aiTextureType_DIFFUSE_ROUGHNESS: return "roughness";
        case aiTextureType_UNKNOWN: return "unknown";
        default: return "none";
    }
}

// All aiTextureType values Phoenix binds textures for, in the same order Phoenix iterates them.
constexpr std::array<aiTextureType, 11> kPhoenixTextureTypes {
    aiTextureType_DIFFUSE,
    aiTextureType_SPECULAR,
    aiTextureType_AMBIENT,
    aiTextureType_HEIGHT,
    aiTextureType_NORMALS,
    aiTextureType_EMISSIVE,
    aiTextureType_SHININESS,
    aiTextureType_AMBIENT_OCCLUSION,
    aiTextureType_METALNESS,
    aiTextureType_DIFFUSE_ROUGHNESS,
    aiTextureType_UNKNOWN,
};

// Resolves a texture path referenced by a material relative to the model's own folder, since
// glTF/OBJ/FBX materials store texture references relative to the source file (matching how
// Phoenix's Material::loadTextures resolves paths relative to the model directory).
std::filesystem::path resolveTexturePath(const aiString& relativePath, const std::filesystem::path& modelDirectory) {
    return modelDirectory / std::filesystem::path(relativePath.C_Str());
}

// Recursively flattens the Assimp scene graph into ModelDocument::sceneNodes, recording each
// node's parent index so SkeletalAnimator can walk parent chains without touching Assimp types.
void flattenNodeHierarchy(const aiNode* node, int parentIndex, ModelDocument& document, std::unordered_map<std::string, int>& nodeIndexByName) {
    ModelDocument::SceneNode sceneNode;
    sceneNode.name = node->mName.C_Str();
    sceneNode.localTransform = toGlmMat4(node->mTransformation);
    sceneNode.parentIndex = parentIndex;

    const int thisIndex = static_cast<int>(document.sceneNodes.size());
    document.sceneNodes.push_back(sceneNode);
    nodeIndexByName[sceneNode.name] = thisIndex;

    for (unsigned int childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
        flattenNodeHierarchy(node->mChildren[childIndex], thisIndex, document, nodeIndexByName);
    }
}

// Registers a bone the first time it is seen and returns its stable index, matching Phoenix's
// approach of building a bone-name -> index map incrementally while walking meshes.
std::size_t boneIndexFor(const aiBone* bone, ModelDocument& document, std::unordered_map<std::string, std::size_t>& boneIndexByName) {
    const std::string name = bone->mName.C_Str();
    const auto existing = boneIndexByName.find(name);
    if (existing != boneIndexByName.end()) {
        return existing->second;
    }
    ModelDocument::BoneInfo info;
    info.name = name;
    info.offsetMatrix = toGlmMat4(bone->mOffsetMatrix);
    const std::size_t newIndex = document.bones.size();
    document.bones.push_back(info);
    boneIndexByName.emplace(name, newIndex);
    return newIndex;
}

// Assigns a bone influence to a vertex in its first free slot (up to kMaxBonesPerVertex),
// mirroring Phoenix's Mesh::setVertexBoneData behavior of ignoring extra low-weight influences
// beyond the first four rather than failing the import.
void addBoneWeightToVertex(ModelVertex& vertex, std::uint32_t boneIndex, float weight) {
    for (int slot = 0; slot < kMaxBonesPerVertex; ++slot) {
        if (vertex.boneWeights[static_cast<std::size_t>(slot)] == 0.0F) {
            vertex.boneIds[static_cast<std::size_t>(slot)] = boneIndex;
            vertex.boneWeights[static_cast<std::size_t>(slot)] = weight;
            return;
        }
    }
    // All four slots already used; Phoenix silently drops additional influences too.
}

ModelMesh convertMesh(const aiMesh* mesh, const aiScene* scene, const std::filesystem::path& modelDirectory,
                      ModelDocument& document, std::unordered_map<std::string, std::size_t>& boneIndexByName) {
    ModelMesh result;
    result.vertices.resize(mesh->mNumVertices);

    for (unsigned int vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
        ModelVertex& vertex = result.vertices[vertexIndex];
        vertex.position = toGlmVec3(mesh->mVertices[vertexIndex]);
        if (mesh->HasNormals()) {
            vertex.normal = toGlmVec3(mesh->mNormals[vertexIndex]);
        }
        if (mesh->HasTextureCoords(0)) {
            vertex.texCoords = {mesh->mTextureCoords[0][vertexIndex].x, mesh->mTextureCoords[0][vertexIndex].y};
        }
        if (mesh->HasTangentsAndBitangents()) {
            vertex.tangent = toGlmVec3(mesh->mTangents[vertexIndex]);
            vertex.biTangent = toGlmVec3(mesh->mBitangents[vertexIndex]);
        }
    }

    result.indices.reserve(static_cast<std::size_t>(mesh->mNumFaces) * 3U);
    for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
        const aiFace& face = mesh->mFaces[faceIndex];
        // aiProcess_Triangulate guarantees every face has exactly 3 indices.
        for (unsigned int cornerIndex = 0; cornerIndex < face.mNumIndices; ++cornerIndex) {
            result.indices.push_back(face.mIndices[cornerIndex]);
        }
    }

    if (mesh->HasBones()) {
        document.hasSkeleton = true;
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            const aiBone* bone = mesh->mBones[boneIndex];
            const std::size_t globalBoneIndex = boneIndexFor(bone, document, boneIndexByName);
            for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
                const aiVertexWeight& weight = bone->mWeights[weightIndex];
                if (weight.mWeight <= 0.0F) {
                    continue;
                }
                addBoneWeightToVertex(result.vertices[weight.mVertexId], static_cast<std::uint32_t>(globalBoneIndex), weight.mWeight);
            }
        }
    }

    if (scene->mNumMaterials > mesh->mMaterialIndex) {
        const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        aiColor3D diffuse(1.0F, 1.0F, 1.0F);
        aiColor3D ambient(0.0F, 0.0F, 0.0F);
        aiColor3D specular(0.0F, 0.0F, 0.0F);
        float specularStrength = 0.0F;
        material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
        material->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
        material->Get(AI_MATKEY_COLOR_SPECULAR, specular);
        material->Get(AI_MATKEY_SHININESS_STRENGTH, specularStrength);
        result.material.colorDiffuse = {diffuse.r, diffuse.g, diffuse.b};
        result.material.colorAmbient = {ambient.r, ambient.g, ambient.b};
        result.material.colorSpecular = {specular.r, specular.g, specular.b};
        result.material.specularStrength = specularStrength;

        for (const aiTextureType textureType : kPhoenixTextureTypes) {
            const unsigned int textureCount = material->GetTextureCount(textureType);
            for (unsigned int textureIndexWithinType = 0; textureIndexWithinType < textureCount; ++textureIndexWithinType) {
                aiString texturePath;
                if (material->GetTexture(textureType, textureIndexWithinType, &texturePath) != AI_SUCCESS) {
                    continue;
                }
                ModelTextureSlot slot;
                // Phoenix names uniforms "texture_" + type + (1-based index within that type).
                slot.shaderUniformName = std::string("texture_") + phoenixTextureTypeName(textureType) +
                                          std::to_string(textureIndexWithinType + 1);
                // Assimp represents embedded textures (common in .glb) with a path of the form
                // "*<index>" referencing scene->mTextures[index], instead of a real file path.
                const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(texturePath.C_Str());
                if (embeddedTexture != nullptr) {
                    if (embeddedTexture->mHeight == 0) {
                        // Compressed image (PNG/JPEG) stored as a raw byte buffer; mWidth is the
                        // byte count. Copy it so PreviewRenderer can decode it independently of
                        // the aiScene's lifetime (the Assimp::Importer is destroyed after load()).
                        const auto* bytes = reinterpret_cast<const unsigned char*>(embeddedTexture->pcData);
                        slot.embeddedImageData.assign(bytes, bytes + embeddedTexture->mWidth);
                    }
                    // Uncompressed (mHeight != 0) embedded textures are rare and unsupported here;
                    // the slot is still added so the uniform exists, just without pixel data.
                } else {
                    slot.sourcePath = resolveTexturePath(texturePath, modelDirectory);
                }
                result.material.textureSlots.push_back(std::move(slot));
            }
        }
    }

    return result;
}

// Imports the cameras authored in the scene (aiScene::mCameras). Assimp names each camera after
// the scene node that carries it, so the node name doubles as the link into the node hierarchy -
// which is what lets a keyframe-animated camera node move the camera (see ModelCameraResolver).
void convertCameras(const aiScene* scene, ModelDocument& document) {
    document.cameras.reserve(scene->mNumCameras);
    for (unsigned int cameraIndex = 0; cameraIndex < scene->mNumCameras; ++cameraIndex) {
        const aiCamera* camera = scene->mCameras[cameraIndex];
        ModelCamera converted;
        converted.name = camera->mName.C_Str();
        converted.nodeName = converted.name;
        converted.position = toGlmVec3(camera->mPosition);
        converted.lookAt = toGlmVec3(camera->mLookAt);
        converted.up = toGlmVec3(camera->mUp);
        converted.horizontalFovRadians = camera->mHorizontalFOV;
        converted.aspectRatio = camera->mAspect;
        converted.nearPlane = camera->mClipPlaneNear;
        converted.farPlane = camera->mClipPlaneFar;
        if (converted.name.empty()) {
            converted.name = "Camera " + std::to_string(cameraIndex);
        }
        document.cameras.push_back(std::move(converted));
    }
}

void convertAnimations(const aiScene* scene, ModelDocument& document) {
    for (unsigned int animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex) {
        const aiAnimation* animation = scene->mAnimations[animationIndex];
        ModelDocument::AnimationClip clip;
        clip.name = animation->mName.C_Str();
        // Convert from Assimp's "ticks" to seconds so SkeletalAnimator can drive playback
        // directly from PlaybackClockState::elapsedSeconds() without unit conversion elsewhere.
        const double ticksPerSecond = animation->mTicksPerSecond != 0.0 ? animation->mTicksPerSecond : 25.0;
        clip.durationSeconds = static_cast<float>(animation->mDuration / ticksPerSecond);

        for (unsigned int channelIndex = 0; channelIndex < animation->mNumChannels; ++channelIndex) {
            const aiNodeAnim* channel = animation->mChannels[channelIndex];
            ModelDocument::AnimationChannel converted;
            converted.boneName = channel->mNodeName.C_Str();

            converted.positionTimes.reserve(channel->mNumPositionKeys);
            converted.positionValues.reserve(channel->mNumPositionKeys);
            for (unsigned int keyIndex = 0; keyIndex < channel->mNumPositionKeys; ++keyIndex) {
                const aiVectorKey& key = channel->mPositionKeys[keyIndex];
                converted.positionTimes.push_back(static_cast<float>(key.mTime / ticksPerSecond));
                converted.positionValues.push_back(toGlmVec3(key.mValue));
            }

            converted.rotationTimes.reserve(channel->mNumRotationKeys);
            converted.rotationValues.reserve(channel->mNumRotationKeys);
            for (unsigned int keyIndex = 0; keyIndex < channel->mNumRotationKeys; ++keyIndex) {
                const aiQuatKey& key = channel->mRotationKeys[keyIndex];
                converted.rotationTimes.push_back(static_cast<float>(key.mTime / ticksPerSecond));
                converted.rotationValues.push_back(toGlmQuat(key.mValue));
            }

            converted.scaleTimes.reserve(channel->mNumScalingKeys);
            converted.scaleValues.reserve(channel->mNumScalingKeys);
            for (unsigned int keyIndex = 0; keyIndex < channel->mNumScalingKeys; ++keyIndex) {
                const aiVectorKey& key = channel->mScalingKeys[keyIndex];
                converted.scaleTimes.push_back(static_cast<float>(key.mTime / ticksPerSecond));
                converted.scaleValues.push_back(toGlmVec3(key.mValue));
            }

            clip.channels.push_back(std::move(converted));
        }

        document.animations.push_back(std::move(clip));
    }
}

// Records the scene-node placement of every mesh reference in the tree. Assimp's node mMeshes are
// indices into aiScene::mMeshes, which is exactly the order document.meshes was built in, so the
// index maps across directly. A mesh may be referenced by many nodes (instancing): each reference
// becomes its own draw instance, while nodeName/nodeIndex keep the first placement so single-node
// meshes keep behaving as before.
void assignMeshNodes(const aiNode* node, ModelDocument& document, const std::unordered_map<std::string, int>& nodeIndexByName) {
    const auto foundNode = nodeIndexByName.find(node->mName.C_Str());
    const int thisNodeIndex = foundNode != nodeIndexByName.end() ? foundNode->second : -1;

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        const unsigned int meshIndex = node->mMeshes[i];
        if (meshIndex >= document.meshes.size()) {
            continue;
        }
        if (document.meshes[meshIndex].nodeName.empty()) {
            document.meshes[meshIndex].nodeName = node->mName.C_Str();
            document.meshes[meshIndex].nodeIndex = thisNodeIndex;
        }
        document.meshInstances.push_back(ModelMeshInstance {meshIndex, thisNodeIndex});
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        assignMeshNodes(node->mChildren[i], document, nodeIndexByName);
    }
}

// True when two materials would produce identical GPU state, so meshes using them can share a
// single set of material-uniform uploads and texture binds.
bool materialsEqual(const ModelMaterial& a, const ModelMaterial& b) {
    if (a.colorAmbient != b.colorAmbient || a.colorDiffuse != b.colorDiffuse || a.colorSpecular != b.colorSpecular ||
        a.specularStrength != b.specularStrength || a.textureSlots.size() != b.textureSlots.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.textureSlots.size(); ++i) {
        const ModelTextureSlot& lhs = a.textureSlots[i];
        const ModelTextureSlot& rhs = b.textureSlots[i];
        if (lhs.shaderUniformName != rhs.shaderUniformName || lhs.sourcePath != rhs.sourcePath ||
            lhs.embeddedImageData != rhs.embeddedImageData) {
            return false;
        }
    }
    return true;
}

// Collapses the per-mesh materials into a deduplicated table, pointing each mesh at its entry.
// Importers hand out one aiMaterial reference per mesh, so a scene like planets.fbx ends up with
// hundreds of meshes referencing only a handful of distinct materials.
void deduplicateMaterials(ModelDocument& document) {
    for (ModelMesh& mesh : document.meshes) {
        int found = -1;
        for (std::size_t i = 0; i < document.materials.size(); ++i) {
            if (materialsEqual(document.materials[i], mesh.material)) {
                found = static_cast<int>(i);
                break;
            }
        }
        if (found < 0) {
            found = static_cast<int>(document.materials.size());
            document.materials.push_back(mesh.material);
        }
        mesh.materialIndex = found;
    }
}
}

ModelLoadResult AssimpModelLoader::load(const std::filesystem::path& modelPath) const {
    ModelLoadResult result;

    Assimp::Importer importer;
    // Post-process flags mirror Phoenix's Model.cpp (see research.md section 5) so imported
    // geometry/bones behave identically to what Phoenix's own loader would produce, including
    // splitting any mesh whose bone count exceeds the supported limit instead of failing.
    constexpr unsigned int postProcessFlags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_SplitByBoneCount |
        aiProcess_FindInstances |
        aiProcess_ValidateDataStructure |
        aiProcess_ImproveCacheLocality |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_FindDegenerates;

    const aiScene* scene = importer.ReadFile(modelPath.string(), postProcessFlags);
    if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0U || scene->mRootNode == nullptr) {
        result.success = false;
        result.errorMessage = "Unable to import model '" + modelPath.string() + "': " + importer.GetErrorString();
        return result;
    }

    ModelDocument& document = result.document;
    document.sourcePath = modelPath;
    const std::filesystem::path modelDirectory = modelPath.parent_path();

    std::unordered_map<std::string, int> nodeIndexByName;
    flattenNodeHierarchy(scene->mRootNode, -1, document, nodeIndexByName);

    std::unordered_map<std::string, std::size_t> boneIndexByName;
    document.meshes.reserve(scene->mNumMeshes);
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        document.meshes.push_back(convertMesh(scene->mMeshes[meshIndex], scene, modelDirectory, document, boneIndexByName));
    }
    assignMeshNodes(scene->mRootNode, document, nodeIndexByName);
    // A mesh no node references would otherwise never be drawn; keep it visible at identity.
    if (document.meshInstances.empty() && !document.meshes.empty()) {
        for (std::size_t meshIndex = 0; meshIndex < document.meshes.size(); ++meshIndex) {
            document.meshInstances.push_back(ModelMeshInstance {meshIndex, -1});
        }
    }
    deduplicateMaterials(document);
    document.boneCount = document.bones.size();

    convertAnimations(scene, document);
    convertCameras(scene, document);
    document.materialCount = scene->mNumMaterials;

    // Compute the bind-pose AABB so the preview camera can scale zoom/orbit/pan proportionally to
    // this model's actual size instead of assuming the ~1-unit scale of the built-in primitives.
    // Instances are placed by their node's static world transform, because a scene built from
    // repeated props keeps every copy at the origin in mesh-local space and would otherwise be
    // measured far smaller than it is drawn. Skinned models are excluded for the same reason the
    // renderer excludes them: their placement comes from gBones, not from the node chain.
    std::vector<glm::mat4> staticWorldTransforms(document.sceneNodes.size(), glm::mat4(1.0F));
    for (std::size_t i = 0; i < document.sceneNodes.size(); ++i) {
        const auto& node = document.sceneNodes[i];
        // Nodes are stored depth-first, so a parent always has a lower index and is already final.
        staticWorldTransforms[i] = node.parentIndex >= 0
                                       ? staticWorldTransforms[static_cast<std::size_t>(node.parentIndex)] * node.localTransform
                                       : node.localTransform;
    }

    bool hasAnyVertex = false;
    glm::vec3 boundsMin {0.0F};
    glm::vec3 boundsMax {0.0F};
    for (const ModelMeshInstance& instance : document.meshInstances) {
        if (instance.meshIndex >= document.meshes.size()) {
            continue;
        }
        glm::mat4 placement(1.0F);
        if (!document.hasSkeleton && instance.nodeIndex >= 0 &&
            static_cast<std::size_t>(instance.nodeIndex) < staticWorldTransforms.size()) {
            placement = staticWorldTransforms[static_cast<std::size_t>(instance.nodeIndex)];
        }
        for (const ModelVertex& vertex : document.meshes[instance.meshIndex].vertices) {
            const glm::vec3 position = glm::vec3(placement * glm::vec4(vertex.position, 1.0F));
            if (!hasAnyVertex) {
                boundsMin = position;
                boundsMax = position;
                hasAnyVertex = true;
            } else {
                boundsMin = glm::min(boundsMin, position);
                boundsMax = glm::max(boundsMax, position);
            }
        }
    }
    document.boundsMin = boundsMin;
    document.boundsMax = boundsMax;

    result.success = true;
    return result;
}
}  // namespace shadereditor
