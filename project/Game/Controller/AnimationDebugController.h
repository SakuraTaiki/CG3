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


    enum class PlayerAnimationState {
        Idle,
        Walk,
        Run,
        Slide
    };

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


    const char* GetPlayerAnimationStateName() const;

    float GetMovementHoldTime() const {
        return movementHoldTime_;
    }

    float GetCurrentAnimationDuration() const {
        return animation_.duration;
    }

    bool IsAnimationPlaying() const {
        return animationPlaying_;
    }

    bool IsAnimationLoop() const {
        return animationLoop_;
    }

    bool IsAnimationBlending() const {
        return isBlending_;
    }

    bool IsManualAnimationTest() const {
        return debugManualAnimation_;
    }

    void SetManualAnimationTest(bool enabled) {
        debugManualAnimation_ = enabled;
    }

    void ForceIdleAnimation();
    void ForceWalkAnimation();
    void ForceRunAnimation();
    void ForceSlideAnimation();

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

    void ChangePlayerAnimation(
        PlayerAnimationState nextState
    );

    void StartAnimationTransition(
        const Animation& nextAnimation,
        float blendDuration,
		bool loop
    );

    void UpdatePlayerMovement(Input* input);


private:
    std::unique_ptr<Object3d> animatedObject_;

    std::vector<std::unique_ptr<Object3d>>
        skeletonDebugObjects_;

    std::vector<std::unique_ptr<Object3d>>
        skeletonBoneObjects_;


    // 読み込んだアニメーション
    Animation walkAnimation_;
    Animation idleAnimation_;
    Animation runAnimation_;
    Animation slideAnimation_;

    // 現在再生中のアニメーション
    Animation animation_;

    PlayerAnimationState playerAnimationState_ =
        PlayerAnimationState::Idle;

    float movementHoldTime_ = 0.0f;

    // Slide後、WASDを一度離すまでIdleを維持
    bool requireMovementRelease_ = false;



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


    Vector3 playerPosition_ = {
       0.0f,
       0.0f,
       0.0f
    };

    float playerFacingYaw_ = 0.0f;

    float playerMoveSpeed_ = 4.0f;
    float playerTurnSpeed_ = 12.0f;

    bool debugManualAnimation_ = false;

};