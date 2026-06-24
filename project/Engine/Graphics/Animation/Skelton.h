#pragma once
#include <vector>
#include <map>
#include <optional>
#include "Model.h"
#include "Animation.h"


struct Joint {

    QuaternionTransform transform;

    Matrix4x4 localMatrix;
    Matrix4x4 skeletonSpaceMatrix;

    std::string name;

    std::vector<int32_t> children;

    int32_t index;

    std::optional<int32_t> parent;
};

struct Skeleton {

    int32_t root;

    std::map<std::string, int32_t> jointMap;

    std::vector<Joint> joints;
};

int32_t CreateJoint(
    const Node& node,
    const std::optional<int32_t>& parent,
    std::vector<Joint>& joints);

Skeleton CreateSkeleton(const Node& rootNode);

void UpdateSkelton(Skeleton& skeleton);

void ApplyAnimation(
    Skeleton& skeleton,
    const Animation& animation,
    float animationTime);
