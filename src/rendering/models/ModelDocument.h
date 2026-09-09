#pragma once

#include <glad/glad.h>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace shadereditor {
// Maximum bones that can influence a single vertex, matching Phoenix's NUM_BONES_PER_VERTEX.
constexpr int kMaxBonesPerVertex = 4;

// One vertex of an imported mesh. Field order and attribute names match Phoenix's
// Mesh::setupMesh layout exactly (aPos/aNormal/aTexCoords/aTangent/aBiTangent/aBoneID/
// aBoneWeight) so bundled Phoenix shaders can be used unmodified against imported models.
struct ModelVertex {
    glm::vec3 position {0.0F};   // aPos
    glm::vec3 normal {0.0F};     // aNormal
    glm::vec2 texCoords {0.0F};  // aTexCoords
    glm::vec3 tangent {0.0F};    // aTangent
    glm::vec3 biTangent {0.0F};  // aBiTangent
    // Bone indices/weights are left at zero/0.0 for meshes without a skeleton (SkeletalAnimator
    // then falls back to an identity transform for such vertices).
    std::array<std::uint32_t, kMaxBonesPerVertex> boneIds {0, 0, 0, 0};  // aBoneID
    std::array<float, kMaxBonesPerVertex> boneWeights {0.0F, 0.0F, 0.0F, 0.0F};  // aBoneWeight
};

// One texture slot bound to a mesh's material, named per Phoenix's texture-uniform convention
// ("texture_" + phoenix type name + 1-based index within that type), e.g. "texture_diffuse1".
struct ModelTextureSlot {
    std::string shaderUniformName;
    // Populated when the texture references an external image file (most FBX/OBJ/DAE content).
    std::filesystem::path sourcePath;
    // Populated instead of sourcePath when the texture is embedded inside the model file itself
    // (common for glTF/.glb, which packs images as binary buffer views rather than loose files).
    // Holds the raw, still-encoded (e.g. PNG/JPEG) file bytes exactly as Assimp exposed them, so
    // PreviewRenderer can decode them with stbi_load_from_memory the same way textureForPath()
    // decodes on-disk files.
    std::vector<unsigned char> embeddedImageData;
    // Populated by PreviewRenderer the first time this texture is uploaded to the GPU; 0 means
    // "not yet uploaded" so the renderer can lazily create/cache GL texture objects per path.
    GLuint glTextureId {0};
};

// Material properties for one mesh, matching the Phoenix uniform names uploaded by
// Mesh::setMaterialShaderVars: Mat_Ka/Mat_Kd/Mat_Ks/Mat_KsStrenght, plus per-type textures.
struct ModelMaterial {
    std::vector<ModelTextureSlot> textureSlots;
    glm::vec3 colorAmbient {0.0F};    // Mat_Ka
    glm::vec3 colorDiffuse {1.0F};    // Mat_Kd
    glm::vec3 colorSpecular {0.0F};   // Mat_Ks
    float specularStrength {0.0F};    // Mat_KsStrenght
    // glTF metallic-roughness workflow factors (AI_MATKEY_METALLIC_FACTOR/ROUGHNESS_FACTOR),
    // uploaded to assets/shaders/pbr_animation.glsl's metallicFactor/roughnessFactor uniforms so
    // PBR glTF imports render with their authored values instead of the shader's 1.0 defaults.
    float metallicFactor {1.0F};
    float roughnessFactor {1.0F};
    // True when this material has a dedicated metalness and/or roughness texture (glTF's
    // metallicRoughnessTexture, split by AssimpModelLoader into separate "texture_metalness1"/
    // "texture_roughness1" slots), matching pbr_animation.glsl's hasPbrTextures uniform.
    bool hasPbrTextures {false};
    // True when a "texture_diffuse1"/"texture_base_color1" slot is present. Some glTF materials
    // (e.g. Khronos's CarConcept "Mechanical" material) carry only a baseColorFactor and no
    // baseColorTexture at all; pbr_animation.glsl uses this to fall back to colorDiffuse instead
    // of sampling an unbound texture unit (which previously produced garbage/black output).
    bool hasDiffuseTexture {false};
    // glTF's KHR_materials_transmission factor (AI_MATKEY_TRANSMISSION_FACTOR): 0 = fully
    // opaque, 1 = fully transmissive (e.g. car glass/windshields). Combined with opacity to
    // derive the alpha the renderer blends with (see bindMeshMaterial/renderFrame blending).
    float transmissionFactor {0.0F};
    // glTF alpha value (AI_MATKEY_OPACITY, 1.0 = fully opaque). Kept separate from
    // transmissionFactor since either can make a material translucent independently.
    float opacity {1.0F};
    // True when a "texture_normal1" slot (glTF normalTexture) is present, matching
    // pbr_animation.glsl's hasNormalMap uniform; falls back to the interpolated vertex normal
    // when false instead of sampling an unbound sampler.
    bool hasNormalMap {false};
    // True when a "texture_emissive1" slot (glTF emissiveTexture) is present, matching
    // pbr_animation.glsl's hasEmissiveTexture uniform.
    bool hasEmissiveTexture {false};
    // glTF emissive color factor (AI_MATKEY_COLOR_EMISSIVE), multiplied with the emissive
    // texture (or used alone when hasEmissiveTexture is false) to add self-illumination, e.g.
    // brake lights/headlights/glowing panels that shouldn't depend on scene lighting at all.
    glm::vec3 emissiveFactor {0.0F};
};

// One (possibly bone-count-split) mesh of an imported model.
struct ModelMesh {
    std::vector<ModelVertex> vertices;
    std::vector<unsigned int> indices;
    ModelMaterial material;
    // Name of the scene node that references this mesh. Node-level keyframe animation (a moving
    // object that carries no skeleton) lives in this node's transform rather than in gBones, so
    // the renderer must fold the node's animated world transform into the mesh's model matrix.
    // Empty means "not referenced by any node", in which case the identity transform is used.
    std::string nodeName;
    // Index of that node into ModelDocument::sceneNodes, resolved once at import time. The
    // renderer looks up a per-frame transform array by this index, so it never has to search the
    // hierarchy by name while drawing (which was O(nodes) per mesh, per frame).
    int nodeIndex {-1};
    // Index into the deduplicated material table below. Meshes sharing a material can be drawn
    // back-to-back without re-uploading material uniforms or rebinding identical textures.
    int materialIndex {-1};
};

// One placement of a mesh in the scene graph. Importers (and aiProcess_FindInstances) let several
// nodes reference the same aiMesh, each with its own transform; a city block modelled from repeated
// props may reference 169 distinct meshes from 7388 nodes. Drawing one mesh per entry in
// ModelDocument::meshes would therefore render each prop exactly once, at a single location, and
// silently drop every other copy — so the renderer iterates these instances instead.
struct ModelMeshInstance {
    std::size_t meshIndex {0};
    // Index into ModelDocument::sceneNodes, or -1 for the identity transform.
    int nodeIndex {-1};
};

// A camera authored inside the model file (aiScene::mCameras). Phoenix exposes the same data
// through Model::setCamera/getSelectedCamera; ShaderEditor keeps it read-only and resolves it to
// view/projection matrices in ModelCameraResolver.
struct ModelCamera {
    std::string name;
    // Name of the scene node carrying this camera. Assimp names the camera after its node, so the
    // node's (possibly animated) world transform is what places the camera in the scene.
    std::string nodeName;
    // Authored, node-local placement. glm::lookAt(position, position + lookAt, up) reproduces
    // Phoenix's corrected processCameras(); the naive lookAt(position, lookAt, up) form produces
    // a mirrored view and is deliberately not used.
    glm::vec3 position {0.0F};
    glm::vec3 lookAt {0.0F, 0.0F, -1.0F};
    glm::vec3 up {0.0F, 1.0F, 0.0F};
    // Assimp reports the *half horizontal* field of view in radians; 0 means "unspecified", in
    // which case the preview falls back to its default vertical FOV.
    float horizontalFovRadians {0.0F};
    // Camera-declared aspect ratio; 0 means "unspecified" so the viewport aspect is used instead.
    float aspectRatio {0.0F};
    float nearPlane {0.0F};
    float farPlane {0.0F};
};

// Engine-agnostic in-memory representation of an imported 3D model, produced by
// AssimpModelLoader and consumed by PreviewRenderer/SkeletalAnimator. Contains no Assimp types,
// keeping the Assimp dependency isolated to AssimpModelLoader's implementation file.
struct ModelDocument {
    std::vector<ModelMesh> meshes;
    // Every (mesh, node) placement to draw. Always populated: a mesh referenced by a single node
    // produces a single instance, so the renderer only ever needs to walk this list.
    std::vector<ModelMeshInstance> meshInstances;
    bool hasSkeleton {false};
    // Total distinct bones across the whole model; sizes the gBones upload array.
    std::size_t boneCount {0};
    std::filesystem::path sourcePath;

    // Axis-aligned bounding box across all mesh vertex positions (bind pose, pre-animation),
    // used by the preview camera to frame/scale zoom, orbit and pan proportionally to the
    // model's actual size instead of assuming the ~1-unit scale of the built-in primitives.
    glm::vec3 boundsMin {0.0F};
    glm::vec3 boundsMax {0.0F};

    // Convenience helpers derived from boundsMin/boundsMax.
    [[nodiscard]] glm::vec3 boundsCenter() const { return (boundsMin + boundsMax) * 0.5F; }
    // Half-diagonal of the AABB; a simple, cheap-to-compute stand-in for a bounding-sphere
    // radius that is guaranteed to enclose every vertex.
    [[nodiscard]] float boundingRadius() const {
        return glm::length(boundsMax - boundsMin) * 0.5F;
    }

    // Data needed by SkeletalAnimator to compute per-frame bone transforms. Kept minimal: the
    // node hierarchy (parent-relative local transforms) and each bone's inverse bind ("offset")
    // matrix, indexed the same way as ModelVertex::boneIds.
    struct BoneInfo {
        std::string name;
        glm::mat4 offsetMatrix {1.0F};
    };
    std::vector<BoneInfo> bones;

    // One animation clip's keyframes for one bone's node, sampled and interpolated by
    // SkeletalAnimator. Times are in seconds (converted from Assimp's ticks at load time).
    struct AnimationChannel {
        std::string boneName;
        std::vector<float> positionTimes;
        std::vector<glm::vec3> positionValues;
        std::vector<float> rotationTimes;
        std::vector<glm::quat> rotationValues;  // quaternion keyframes (avoids Euler gimbal issues)
        std::vector<float> scaleTimes;
        std::vector<glm::vec3> scaleValues;
    };
    struct AnimationClip {
        std::string name;
        float durationSeconds {0.0F};
        std::vector<AnimationChannel> channels;
    };
    std::vector<AnimationClip> animations;

    // Node hierarchy used to compose each bone's final transform (node's local transform times
    // its parent chain), mirroring how Phoenix walks the Assimp scene graph in Model::Draw.
    struct SceneNode {
        std::string name;
        glm::mat4 localTransform {1.0F};
        int parentIndex {-1};
    };
    std::vector<SceneNode> sceneNodes;

    // Cameras authored in the model file, in import order so an index stays stable for the
    // lifetime of the document (this index is what RenderSession::activeCameraIndex refers to).
    std::vector<ModelCamera> cameras;

    // Deduplicated materials shared by the meshes above (ModelMesh::materialIndex points here).
    // Assimp reports a material per mesh even when hundreds of meshes share one, so collapsing
    // them lets the renderer sort draws by material and skip redundant uniform/texture binds.
    std::vector<ModelMaterial> materials;

    // Statistics Assimp reported for the imported scene, mirroring Phoenix's m_statNum* counters.
    // Kept here (rather than recomputed) for values that cannot be derived from the converted
    // meshes alone, such as the material count before redundant-material removal.
    std::size_t materialCount {0};
};
}  // namespace shadereditor
