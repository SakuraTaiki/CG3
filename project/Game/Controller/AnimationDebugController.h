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
    enum class PlayerAnimationState {
        Idle,
        Walk,
        Run,
        Slide,
        Jump
    };

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

    const char* GetPlayerAnimationStateName() const;
    float GetMovementHoldTime() const { return movementHoldTime_; }
    float GetCurrentAnimationDuration() const { return animation_.duration; }
    bool IsAnimationPlaying() const { return animationPlaying_; }
    bool IsAnimationLoop() const { return animationLoop_; }
    bool IsAnimationBlending() const { return isBlending_; }
    bool IsManualAnimationTest() const { return manualAnimationTest_; }
    void SetManualAnimationTest(bool enabled) { manualAnimationTest_ = enabled; }

    void ForceIdleAnimation();
    void ForceWalkAnimation();
    void ForceRunAnimation();
    void ForceSlideAnimation();
    void ForceJumpAnimation();

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
        float blendDuration,
        bool loop
    );

    void ChangePlayerAnimation(PlayerAnimationState nextState);

private:
    std::unique_ptr<Object3d> animatedObject_;

    std::vector<std::unique_ptr<Object3d>>
        skeletonDebugObjects_;

    std::vector<std::unique_ptr<Object3d>>
        skeletonBoneObjects_;

    // 読み込んだアニメーション
    Animation idleAnimation_;
    Animation walkAnimation_;
    Animation runAnimation_;
    Animation slideAnimation_;
    Animation jumpAnimation_;

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

    PlayerAnimationState playerAnimationState_ =
        PlayerAnimationState::Idle;
    float movementHoldTime_ = 0.0f;
    bool requireMovementRelease_ = false;
    bool manualAnimationTest_ = false;

    // Jump is moved in world space. The glTF only controls the body pose.
    bool isJumping_ = false;
    float jumpVelocity_ = 0.0f;
    float groundHeight_ = 0.0f;
    float jumpInitialVelocity_ = 15.0f;
    float jumpGravity_ = 30.0f;

    int selectedJointIndex_ = 0;
    bool autoPauseOnJointEdit_ = true;
};
