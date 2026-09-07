#pragma once

#include <glad/glad.h>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
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
    std::filesystem::path sourcePath;
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
};

// One (possibly bone-count-split) mesh of an imported model.
struct ModelMesh {
    std::vector<ModelVertex> vertices;
    std::vector<unsigned int> indices;
    ModelMaterial material;
};

// Engine-agnostic in-memory representation of an imported 3D model, produced by
// AssimpModelLoader and consumed by PreviewRenderer/SkeletalAnimator. Contains no Assimp types,
// keeping the Assimp dependency isolated to AssimpModelLoader's implementation file.
struct ModelDocument {
    std::vector<ModelMesh> meshes;
    bool hasSkeleton {false};
    // Total distinct bones across the whole model; sizes the gBones upload array.
    std::size_t boneCount {0};
    std::filesystem::path sourcePath;

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
};
}  // namespace shadereditor
