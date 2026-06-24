#pragma once

#include <memory>
#include <vector>

#include "Animation.h"
#include "Object3d.h"
#include "Skelton.h"

class Object3dCommon;


class AnimationDebugController
{

public:
    void Initialize(
        Object3dCommon* object3dCommon,
        uint32_t environmentTextureHandle,
        float environmentCoefficient
    );

    void Finalize();

    void Update();
    void Draw();
    void DrawImGui();

    int GetJointCount() const;
    float GetAnimationTime() const { return animationTime_; }

    bool& ShowSkeletonDebug() { return showSkeletonDebug_; }

    void SetEnvironmentCoefficient(float environmentCoefficient);

private:
    void InitializeSkeletonDebug(
        Object3dCommon* object3dCommon,
        uint32_t environmentTextureHandle,
        float environmentCoefficient
    );

    void UpdateAnimation();
    void UpdateSkeletonDebug();
    void SyncSkeletonToObject();

private:
    std::unique_ptr<Object3d> animatedObject_;

    std::vector<std::unique_ptr<Object3d>> skeletonDebugObjects_;
    std::vector<std::unique_ptr<Object3d>> skeletonBoneObjects_;

    Animation animation_;
    Skeleton skeleton_;

    float animationTime_ = 0.0f;

    bool showSkeletonDebug_ = true;
    bool animationPlaying_ = true;
    bool animationLoop_ = true;
    float animationSpeed_ = 1.0f;

    int selectedJointIndex_ = 0;
    bool autoPauseOnJointEdit_ = true;

};

