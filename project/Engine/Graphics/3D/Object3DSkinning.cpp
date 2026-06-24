#include "Object3dSkinning.h"

#include "Object3dCommon.h"
#include "Model.h"

#include <cmath>

void Object3dSkinning::Initialize(Object3dCommon* object3dCommon, Model* model) {
    object3dCommon_ = object3dCommon;
    model_ = model;

    hasSkinCluster_ = false;

    if (!object3dCommon_ || !model_) {
        return;
    }

    // Model の Node 階層から Skeleton を作成する。
    skeleton_ = CreateSkeleton(model_->GetRootNode());

    // Model に skin 情報があれば SkinCluster を作成する。
    hasSkinCluster_ = CreateSkinCluster(
        skinCluster_,
        object3dCommon_->GetDxCommon()->GetDevice(),
        object3dCommon_->GetSrvManager(),
        skeleton_,
        *model_
    );
}

void Object3dSkinning::SetSkeleton(const Skeleton& skeleton) {
    skeleton_ = skeleton;
}

void Object3dSkinning::SetAnimation(const Animation& animation) {
    animation_ = animation;
    useAnimation_ = true;
    animationTime_ = 0.0f;
}

void Object3dSkinning::Update() {
    if (!model_ || !hasSkinCluster_) {
        return;
    }

    // Animation がある場合は時間を進めて Skeleton に反映する。
    if (useAnimation_) {
        animationTime_ += 1.0f / 60.0f;

        if (animation_.duration > 0.0f) {
            animationTime_ = std::fmod(animationTime_, animation_.duration);
        }

        ApplyAnimation(skeleton_, animation_, animationTime_);
    }

    UpdateSkelton(skeleton_);
    UpdateSkinCluster(skinCluster_, skeleton_);
}

Matrix4x4 Object3dSkinning::GetRootLocalMatrix() const {
    if (!model_) {
        return Math::MakeIdentity4x4();
    }

    Matrix4x4 localMatrix = model_->GetRootNode().localMatrix;

    if (!useAnimation_) {
        return localMatrix;
    }

    auto it = animation_.nodeAnimations.find(model_->GetRootNode().name);

    if (it == animation_.nodeAnimations.end()) {
        return localMatrix;
    }

    const NodeAnimation& rootNodeAnimation = it->second;

    Vector3 translate =
        CalculateValue(rootNodeAnimation.translate, animationTime_);

    Quaternion rotate =
        CalculateValue(rootNodeAnimation.rotate, animationTime_);

    Vector3 scale =
        CalculateValue(rootNodeAnimation.scale, animationTime_);

    return Math::MakeAffineMatrix(scale, rotate, translate);
}