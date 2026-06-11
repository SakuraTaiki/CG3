#pragma once
#include <vector>
#include <string>
#include <map>
#include "MyMath.h"

template <typename tValue>
struct Keyframe {
    float time;
    tValue value;
};

using KeyframeVector3 = Keyframe<Vector3>;
using KeyframeQuaternion = Keyframe<Quaternion>;

struct NodeAnimation {
    std::vector<KeyframeVector3> translate;
    std::vector<KeyframeQuaternion> rotate;
    std::vector<KeyframeVector3> scale;
};

struct Animation {
    float duration = 0.0f;
    std::map<std::string, NodeAnimation> nodeAnimations;
};

Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename);

Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time);
Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time);