#include "rendering/models/SkeletalAnimator.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <type_traits>
#include <unordered_map>

namespace shadereditor {
namespace {
// Finds the two keyframes surrounding `time` in a sorted time array and returns the linear
// interpolation factor between them. Falls back to the last/first key when time is out of range
// (looping is handled by the caller via fmod against the clip duration).
template <typename ValueType>
ValueType interpolateKeyframes(const std::vector<float>& times, const std::vector<ValueType>& values, float time, ValueType fallback) {
    if (values.empty()) {
        return fallback;
    }
    if (values.size() == 1 || time <= times.front()) {
        return values.front();
    }
    if (time >= times.back()) {
        return values.back();
    }

    const auto nextIt = std::upper_bound(times.begin(), times.end(), time);
    const std::size_t nextIndex = static_cast<std::size_t>(std::distance(times.begin(), nextIt));
    const std::size_t previousIndex = nextIndex - 1;
    const float span = times[nextIndex] - times[previousIndex];
    const float factor = span > 0.0F ? (time - times[previousIndex]) / span : 0.0F;

    if constexpr (std::is_same_v<ValueType, glm::quat>) {
        return glm::slerp(values[previousIndex], values[nextIndex], factor);
    } else {
        return glm::mix(values[previousIndex], values[nextIndex], factor);
    }
}

// Reconstructs the local transform (translation * rotation * scale) a node should have at
// `time` for a given animation channel, matching how Phoenix samples aiNodeAnim keyframes.
glm::mat4 sampleChannel(const ModelDocument::AnimationChannel& channel, float time) {
    const glm::vec3 position = interpolateKeyframes(channel.positionTimes, channel.positionValues, time, glm::vec3(0.0F));
    const glm::quat rotation = interpolateKeyframes(channel.rotationTimes, channel.rotationValues, time, glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    const glm::vec3 scale = interpolateKeyframes(channel.scaleTimes, channel.scaleValues, time, glm::vec3(1.0F));

    const glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0F), position);
    const glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
    const glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0F), scale);
    return translationMatrix * rotationMatrix * scaleMatrix;
}
}

std::vector<glm::mat4> SkeletalAnimator::boneTransforms(const ModelDocument& document, float elapsedSeconds) const {
    std::vector<glm::mat4> result(document.boneCount, glm::mat4(1.0F));
    if (!document.hasSkeleton || document.sceneNodes.empty()) {
        return result;
    }

    // This example animator plays the model's first animation clip on a loop; supporting clip
    // selection/blending is out of scope for this iteration (see spec Assumptions).
    const ModelDocument::AnimationClip* clip = document.animations.empty() ? nullptr : &document.animations.front();
    float animationTime = 0.0F;
    std::unordered_map<std::string, const ModelDocument::AnimationChannel*> channelByNodeName;
    if (clip != nullptr && clip->durationSeconds > 0.0F) {
        // Loop the clip by wrapping elapsed time into [0, durationSeconds).
        animationTime = std::fmod(elapsedSeconds, clip->durationSeconds);
        if (animationTime < 0.0F) {
            animationTime += clip->durationSeconds;
        }
        for (const auto& channel : clip->channels) {
            channelByNodeName[channel.boneName] = &channel;
        }
    }

    // Compute each node's global (model-space) transform by walking from the root down,
    // multiplying each node's local transform (animated, if a channel exists for it) by its
    // parent's already-computed global transform - the same hierarchy walk Phoenix performs.
    std::vector<glm::mat4> globalTransforms(document.sceneNodes.size(), glm::mat4(1.0F));
    for (std::size_t nodeIndex = 0; nodeIndex < document.sceneNodes.size(); ++nodeIndex) {
        const auto& node = document.sceneNodes[nodeIndex];
        const auto channelIt = channelByNodeName.find(node.name);
        const glm::mat4 localTransform = channelIt != channelByNodeName.end()
                                              ? sampleChannel(*channelIt->second, animationTime)
                                              : node.localTransform;
        // Scene nodes are stored in depth-first order (see AssimpModelLoader::flattenNodeHierarchy),
        // so a node's parent always has a lower index and is already resolved by this point.
        globalTransforms[nodeIndex] = node.parentIndex >= 0
                                           ? globalTransforms[static_cast<std::size_t>(node.parentIndex)] * localTransform
                                           : localTransform;
    }

    // Build a bone name -> scene node index lookup once, then combine each bone's global node
    // transform with its inverse bind ("offset") matrix, exactly as Phoenix's Model::Draw does
    // before uploading the flat "gBones" array.
    std::unordered_map<std::string, std::size_t> nodeIndexByName;
    for (std::size_t nodeIndex = 0; nodeIndex < document.sceneNodes.size(); ++nodeIndex) {
        nodeIndexByName[document.sceneNodes[nodeIndex].name] = nodeIndex;
    }

    for (std::size_t boneIndex = 0; boneIndex < document.bones.size(); ++boneIndex) {
        const auto& bone = document.bones[boneIndex];
        const auto nodeIt = nodeIndexByName.find(bone.name);
        const glm::mat4 nodeGlobalTransform = nodeIt != nodeIndexByName.end() ? globalTransforms[nodeIt->second] : glm::mat4(1.0F);
        result[boneIndex] = nodeGlobalTransform * bone.offsetMatrix;
    }

    return result;
}
}  // namespace shadereditor
