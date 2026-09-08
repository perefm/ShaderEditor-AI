#include "rendering/models/ModelInfoSummary.h"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace shadereditor {
namespace {
// Texture uniform names follow Phoenix's "texture_" + type + 1-based index convention (see
// AssimpModelLoader::phoenixTextureTypeName), so the type is recoverable by stripping the prefix
// and the trailing digits.
std::string textureTypeFromUniformName(const std::string& uniformName) {
    constexpr std::string_view prefix {"texture_"};
    std::string type = uniformName.rfind(prefix, 0) == 0 ? uniformName.substr(prefix.size()) : uniformName;
    while (!type.empty() && (std::isdigit(static_cast<unsigned char>(type.back())) != 0)) {
        type.pop_back();
    }
    return type.empty() ? uniformName : type;
}

// True when any channel of any clip targets this camera's node, i.e. the camera moves over time.
bool isCameraAnimated(const ModelDocument& document, const ModelCamera& camera) {
    return std::any_of(document.animations.begin(), document.animations.end(), [&camera](const ModelDocument::AnimationClip& clip) {
        return std::any_of(clip.channels.begin(), clip.channels.end(), [&camera](const ModelDocument::AnimationChannel& channel) {
            return channel.boneName == camera.nodeName;
        });
    });
}
}

ModelInfoSummary buildModelInfoSummary(const ModelDocument& document) {
    ModelInfoSummary summary;
    if (document.meshes.empty() && document.sourcePath.empty()) {
        return summary;
    }

    summary.hasModel = true;
    summary.name = document.sourcePath.filename().string();
    summary.sourcePath = document.sourcePath.string();

    summary.meshCount = document.meshes.size();
    for (const ModelMesh& mesh : document.meshes) {
        summary.vertexCount += mesh.vertices.size();
        summary.indexCount += mesh.indices.size();
        for (const ModelTextureSlot& slot : mesh.material.textureSlots) {
            ++summary.textureCount;
            if (!slot.embeddedImageData.empty()) {
                ++summary.embeddedTextureCount;
            }
            const std::string type = textureTypeFromUniformName(slot.shaderUniformName);
            const auto existing = std::find_if(summary.texturesByType.begin(), summary.texturesByType.end(),
                                               [&type](const auto& entry) { return entry.first == type; });
            if (existing != summary.texturesByType.end()) {
                ++existing->second;
            } else {
                summary.texturesByType.emplace_back(type, 1U);
            }
        }
    }
    // aiProcess_Triangulate guarantees every face is a triangle, so indices always divide by 3.
    summary.triangleCount = summary.indexCount / 3U;
    summary.hasTextures = summary.textureCount > 0;
    // materialCount is what Assimp reported for the scene; fall back to the mesh count only when
    // the document predates that field (e.g. hand-built test fixtures).
    summary.materialCount = document.materialCount > 0 ? document.materialCount : 0U;

    summary.hasSkeleton = document.hasSkeleton;
    summary.boneCount = document.boneCount;
    summary.sceneNodeCount = document.sceneNodes.size();

    summary.animations.reserve(document.animations.size());
    for (const ModelDocument::AnimationClip& clip : document.animations) {
        summary.animations.push_back({clip.name, clip.durationSeconds, clip.channels.size()});
    }

    summary.cameras.reserve(document.cameras.size());
    for (const ModelCamera& camera : document.cameras) {
        summary.cameras.push_back({camera.name, isCameraAnimated(document, camera)});
    }

    summary.boundsMin = document.boundsMin;
    summary.boundsMax = document.boundsMax;
    summary.boundsSize = document.boundsMax - document.boundsMin;
    summary.boundingRadius = document.boundingRadius();

    return summary;
}
}  // namespace shadereditor
