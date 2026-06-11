#include "Animation.h"
#include <cassert>
#include <cmath>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename) {
    Animation animation{};

    Assimp::Importer importer;

    std::string filePath = directoryPath + "/" + filename;

    const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);

    assert(scene);
    assert(scene->mNumAnimations != 0);

    aiAnimation* animationAssimp = scene->mAnimations[0];

    float ticksPerSecond = static_cast<float>(animationAssimp->mTicksPerSecond);
    if (ticksPerSecond == 0.0f) {
        ticksPerSecond = 1.0f;
    }

    animation.duration =
        static_cast<float>(animationAssimp->mDuration / ticksPerSecond);

    for (uint32_t channelIndex = 0;
        channelIndex < animationAssimp->mNumChannels;
        ++channelIndex)
    {
        aiNodeAnim* nodeAnimationAssimp =
            animationAssimp->mChannels[channelIndex];

        NodeAnimation& nodeAnimation =
            animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

        for (uint32_t keyIndex = 0;
            keyIndex < nodeAnimationAssimp->mNumPositionKeys;
            ++keyIndex)
        {
            aiVectorKey& keyAssimp =
                nodeAnimationAssimp->mPositionKeys[keyIndex];

            KeyframeVector3 keyframe{};
            keyframe.time =
                static_cast<float>(keyAssimp.mTime / ticksPerSecond);

            keyframe.value = {
                -keyAssimp.mValue.x,
                 keyAssimp.mValue.y,
                 keyAssimp.mValue.z
            };

            nodeAnimation.translate.push_back(keyframe);
        }

        for (uint32_t keyIndex = 0;
            keyIndex < nodeAnimationAssimp->mNumRotationKeys;
            ++keyIndex)
        {
            aiQuatKey& keyAssimp =
                nodeAnimationAssimp->mRotationKeys[keyIndex];

            KeyframeQuaternion keyframe{};
            keyframe.time =
                static_cast<float>(keyAssimp.mTime / ticksPerSecond);

            keyframe.value = {
                 keyAssimp.mValue.x,
                -keyAssimp.mValue.y,
                -keyAssimp.mValue.z,
                 keyAssimp.mValue.w
            };

            keyframe.value = Math::Normalize(keyframe.value);

            nodeAnimation.rotate.push_back(keyframe);
        }

        for (uint32_t keyIndex = 0;
            keyIndex < nodeAnimationAssimp->mNumScalingKeys;
            ++keyIndex)
        {
            aiVectorKey& keyAssimp =
                nodeAnimationAssimp->mScalingKeys[keyIndex];

            KeyframeVector3 keyframe{};
            keyframe.time =
                static_cast<float>(keyAssimp.mTime / ticksPerSecond);

            keyframe.value = {
                keyAssimp.mValue.x,
                keyAssimp.mValue.y,
                keyAssimp.mValue.z
            };

            nodeAnimation.scale.push_back(keyframe);
        }
    }

    return animation;
}

Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time) {
    assert(!keyframes.empty());

    if (keyframes.size() == 1 || time <= keyframes[0].time) {
        return keyframes[0].value;
    }

    for (size_t index = 0; index < keyframes.size() - 1; ++index) {
        size_t nextIndex = index + 1;

        if (keyframes[index].time <= time &&
            time <= keyframes[nextIndex].time)
        {
            float t =
                (time - keyframes[index].time) /
                (keyframes[nextIndex].time - keyframes[index].time);

            return Math::Lerp(
                keyframes[index].value,
                keyframes[nextIndex].value,
                t
            );
        }
    }

    return keyframes.back().value;
}

Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time) {
    assert(!keyframes.empty());

    if (keyframes.size() == 1 || time <= keyframes[0].time) {
        return keyframes[0].value;
    }

    for (size_t index = 0; index < keyframes.size() - 1; ++index) {
        size_t nextIndex = index + 1;

        if (keyframes[index].time <= time &&
            time <= keyframes[nextIndex].time)
        {
            float t =
                (time - keyframes[index].time) /
                (keyframes[nextIndex].time - keyframes[index].time);

            return Math::Slerp(
                keyframes[index].value,
                keyframes[nextIndex].value,
                t
            );
        }
    }

    return keyframes.back().value;
}