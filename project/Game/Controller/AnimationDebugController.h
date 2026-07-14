#pragma once

#include <memory>
#include <vector>

#include "Animation.h"
#include "Object3d.h"
#include "Skelton.h"

class Object3dCommon;
class Input;

class AnimationDebugController
{
public:
    void Initialize(
        Object3dCommon* object3dCommon,
        uint32_t environmentTextureHandle,
        float environmentCoefficient
    );

    void Finalize();

    void Update(Input* input);
    void Draw();
    void DrawImGui();

    int GetJointCount() const;
    float GetAnimationTime() const {
        return animationTime_;
    }

    bool& ShowSkeletonDebug() {
        return showSkeletonDebug_;
    }

    void SetEnvironmentCoefficient(
        float environmentCoefficient
    );

private:
    void InitializeSkeletonDebug(
        Object3dCommon* object3dCommon,
        uint32_t environmentTextureHandle,
        float environmentCoefficient
    );

    void UpdateAnimationInput(Input* input);
    void UpdateAnimation();
    void UpdateSkeletonDebug();
    void SyncSkeletonToObject();

    void StartAnimationTransition(
        const Animation& nextAnimation,
        float blendDuration
    );

private:
    std::unique_ptr<Object3d> animatedObject_;

    std::vector<std::unique_ptr<Object3d>>
        skeletonDebugObjects_;

    std::vector<std::unique_ptr<Object3d>>
        skeletonBoneObjects_;

    // 読み込んだアニメーション
    Animation walkAnimation_;
    Animation sneakAnimation_;

    // 現在再生中のアニメーション
    Animation animation_;

    Skeleton skeleton_;

    float animationTime_ = 0.0f;

    bool showSkeletonDebug_ = false;
    bool animationPlaying_ = true;
    bool animationLoop_ = true;
    float animationSpeed_ = 1.0f;

    // クロスフェード開始時の姿勢
    std::vector<QuaternionTransform> blendStartPose_;

    bool isBlending_ = false;
    float blendTime_ = 0.0f;
    float blendDuration_ = 0.25f;

    int selectedJointIndex_ = 0;
    bool autoPauseOnJointEdit_ = true;
};