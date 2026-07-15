#include "AnimationDebugController.h"
#include "AnimationLoader.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "Input.h"

#include <algorithm>
#include <cmath>
#include <string>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void AnimationDebugController::Initialize(
    Object3dCommon* object3dCommon,
    uint32_t environmentTextureHandle,
    float environmentCoefficient
) {
    Model* modelAnimated =
        ModelManager::Load("Resources/human", "idle.gltf");


    idleAnimation_ =
        AnimationLoader::Load(
            "Resources/human",
            "idle.gltf"
        );

    
    
    walkAnimation_ =
        AnimationLoader::Load(
            "Resources/human",
            "walk.gltf"
        );

    runAnimation_ =
        AnimationLoader::Load(
            "Resources/human",
            "run.gltf"
        );

    slideAnimation_ =
        AnimationLoader::Load(
            "Resources/human",
            "slide.gltf"
        );

    jumpAnimation_ =
        AnimationLoader::Load(
            "Resources/human",
            "jump.gltf"
        );

    // 初期状態はIdle
    animation_ = idleAnimation_;
    animationTime_ = 0.0f;
    animationPlaying_ = true;
    animationLoop_ = true;

    playerAnimationState_ =
        PlayerAnimationState::Idle;

    movementHoldTime_ = 0.0f;
    requireMovementRelease_ = false;

    skeleton_ =
        CreateSkeleton(modelAnimated->GetRootNode());

    InitializeSkeletonDebug(
        object3dCommon,
        environmentTextureHandle,
        environmentCoefficient
    );

    animatedObject_ = std::make_unique<Object3d>();
    animatedObject_->Initialize(object3dCommon);
    animatedObject_->SetModel(modelAnimated);
    animatedObject_->SetSkeleton(skeleton_);
   

    animatedObject_->SetPosition({ 0.0f, 0.0f, 0.0f });
    animatedObject_->SetRotation({ -90.0f, 0.0f, 0.0f });
    animatedObject_->SetScale({ 200.0f, 200.0f, 200.0f });

    animatedObject_->SetEnvironmentTexture(environmentTextureHandle);
    animatedObject_->SetEnvironmentCoefficient(environmentCoefficient);
}

void AnimationDebugController::Finalize() {
    animatedObject_.reset();
    skeletonDebugObjects_.clear();
    skeletonBoneObjects_.clear();
}

void AnimationDebugController::InitializeSkeletonDebug(
    Object3dCommon* object3dCommon,
    uint32_t environmentTextureHandle,
    float environmentCoefficient
) {
    skeletonDebugObjects_.clear();
    skeletonBoneObjects_.clear();

    Model* jointModel =
        ModelManager::Load("axis.obj");

    for (size_t i = 0; i < skeleton_.joints.size(); ++i) {
        auto jointObj = std::make_unique<Object3d>();
        jointObj->Initialize(object3dCommon);
        jointObj->SetModel(jointModel);
        jointObj->SetScale({ 0.01f, 0.01f, 0.01f });
        jointObj->SetRotation({ 0.0f, 0.0f, 0.0f });
        jointObj->SetPosition({ 0.0f, 0.0f, 0.0f });
        jointObj->SetEnvironmentTexture(environmentTextureHandle);
        jointObj->SetEnvironmentCoefficient(environmentCoefficient);

        skeletonDebugObjects_.push_back(std::move(jointObj));
    }

    for (const Joint& joint : skeleton_.joints) {
        if (!joint.parent) {
            continue;
        }

        auto boneObj = std::make_unique<Object3d>();
        boneObj->Initialize(object3dCommon);
        boneObj->SetModel(jointModel);
        boneObj->SetScale({ 0.005f, 0.005f, 0.005f });
        boneObj->SetRotation({ 0.0f, 0.0f, 0.0f });
        boneObj->SetPosition({ 0.0f, 0.0f, 0.0f });
        boneObj->SetEnvironmentTexture(environmentTextureHandle);
        boneObj->SetEnvironmentCoefficient(environmentCoefficient);

        skeletonBoneObjects_.push_back(std::move(boneObj));
    }
}

void AnimationDebugController::Update(Input* input)
{
    UpdateAnimationInput(input);
    UpdatePlayerMovement(input);
    UpdateAnimation();

    if (showSkeletonDebug_) {
        UpdateSkeletonDebug();
    }

    SyncSkeletonToObject();

    if (animatedObject_) {
        animatedObject_->Update();
    }
}



void AnimationDebugController::UpdateAnimationInput(Input* input)
{

    if (!input) {
        return;
    }

    // ImGuiで手動確認中はWASDによる切り替えを停止
    if (debugManualAnimation_) {
        return;
    }


    constexpr float deltaTime = 1.0f / 60.0f;
    constexpr float runStartSeconds = 2.0f;


    const bool movementPressed =
        input->PushKey(DIK_W) ||
        input->PushKey(DIK_A) ||
        input->PushKey(DIK_S) ||
        input->PushKey(DIK_D);

    const bool jumpTriggered =
        input->TriggerKey(DIK_RETURN);

    const bool slideTriggered =
        input->TriggerKey(DIK_LCONTROL) ||
        input->TriggerKey(DIK_RCONTROL);


    // Jump・Slideは単発再生が終わるまで切り替えない
    if (
        playerAnimationState_ ==
        PlayerAnimationState::Jump ||
        playerAnimationState_ ==
        PlayerAnimationState::Slide
        ) {
        return;
    }

    // SpaceでJump
    if (jumpTriggered) {
        movementHoldTime_ = 0.0f;

        ChangePlayerAnimation(
            PlayerAnimationState::Jump
        );

        return;
    }

    // Slide後はWASDを一度離すまでIdle
    if (requireMovementRelease_) {
        if (movementPressed) {
            ChangePlayerAnimation(
                PlayerAnimationState::Idle
            );
            return;
        }

        requireMovementRelease_ = false;
    }


    // 移動中にCtrlでSlide
    if (slideTriggered && movementPressed) {
        movementHoldTime_ = 0.0f;
        requireMovementRelease_ = true;

        ChangePlayerAnimation(
            PlayerAnimationState::Slide
        );

        return;
    }

    // 移動入力がなければIdle
    if (!movementPressed) {
        movementHoldTime_ = 0.0f;

        ChangePlayerAnimation(
            PlayerAnimationState::Idle
        );

        return;
    }

    movementHoldTime_ += deltaTime;

    // 最初の2秒はWalk
    if (movementHoldTime_ < runStartSeconds) {
        ChangePlayerAnimation(
            PlayerAnimationState::Walk
        );
    }
    // 2秒以上ならRun
    else {
        ChangePlayerAnimation(
            PlayerAnimationState::Run
        );
    }

}


void AnimationDebugController::UpdateAnimation()
{
    constexpr float deltaTime = 1.0f / 60.0f;

    // アニメーション時間を進める
    if (animationPlaying_) {
        animationTime_ +=
            deltaTime * animationSpeed_;

        if (animation_.duration > 0.0f) {
            if (animationLoop_) {
                animationTime_ =
                    std::fmod(
                        animationTime_,
                        animation_.duration
                    );
            } else if (
                animationTime_ >=
                animation_.duration
                ) {
                animationTime_ =
                    animation_.duration;

                animationPlaying_ = false;
            }
        }
    }

    
    if (
        (
            playerAnimationState_ ==
            PlayerAnimationState::Jump ||
            playerAnimationState_ ==
            PlayerAnimationState::Slide
            ) &&
        !animationPlaying_
        ) {
        playerAnimationState_ =
            PlayerAnimationState::Idle;

        movementHoldTime_ = 0.0f;

        StartAnimationTransition(
            idleAnimation_,
            0.18f,
            true
        );
    }


    // クロスフェード中
    if (isBlending_) {
        // 切り替え先アニメーションの姿勢を計算
        Skeleton targetSkeleton = skeleton_;

        ApplyAnimation(
            targetSkeleton,
            animation_,
            animationTime_
        );

        blendTime_ += deltaTime;

        float blend =
            std::clamp(
                blendTime_ / blendDuration_,
                0.0f,
                1.0f
            );

        // Smoothstepで開始と終了を滑らかにする
        blend =
            blend *
            blend *
            (3.0f - 2.0f * blend);

        const size_t jointCount =
            (std::min)(
                skeleton_.joints.size(),
                blendStartPose_.size()
                );

        for (
            size_t i = 0;
            i < jointCount;
            ++i
            ) {
            const QuaternionTransform& start =
                blendStartPose_[i];

            const QuaternionTransform& target =
                targetSkeleton
                .joints[i]
                .transform;

            // 平行移動
            skeleton_
                .joints[i]
                .transform
                .translate =
                Math::Lerp(
                    start.translate,
                    target.translate,
                    blend
                );

            // 回転
            skeleton_
                .joints[i]
                .transform
                .rotate =
                Math::Slerp(
                    start.rotate,
                    target.rotate,
                    blend
                );

            // スケール
            skeleton_
                .joints[i]
                .transform
                .scale =
                Math::Lerp(
                    start.scale,
                    target.scale,
                    blend
                );
        }

        // クロスフェード完了
        if (blendTime_ >= blendDuration_) {
            isBlending_ = false;

            ApplyAnimation(
                skeleton_,
                animation_,
                animationTime_
            );
        }
    } else if (animationPlaying_) {
        // 通常再生
        ApplyAnimation(
            skeleton_,
            animation_,
            animationTime_
        );
    }

    // 各Jointの行列を更新
    UpdateSkelton(skeleton_);
}


void AnimationDebugController::SyncSkeletonToObject() {
    if (!animatedObject_) {
        return;
    }

    animatedObject_->SetSkeleton(skeleton_);
}

void AnimationDebugController::ChangePlayerAnimation(
    PlayerAnimationState nextState
) {
    // 同じ状態ならanimationTime_をリセットしない
    if (playerAnimationState_ == nextState) {
        return;
    }

    playerAnimationState_ = nextState;

    switch (nextState) {
    case PlayerAnimationState::Idle:
        StartAnimationTransition(
            idleAnimation_, 0.20f, true
        );
        break;

    case PlayerAnimationState::Walk:
        StartAnimationTransition(
            walkAnimation_, 0.20f, true
        );
        break;

    case PlayerAnimationState::Run:
        StartAnimationTransition(
            runAnimation_, 0.20f, true
        );
        break;

    case PlayerAnimationState::Jump:
        StartAnimationTransition(
            jumpAnimation_,
            0.10f,
            false
        );
        break;

    case PlayerAnimationState::Slide:
        StartAnimationTransition(
            slideAnimation_, 0.12f, false
        );
        break;
    }
}

void AnimationDebugController::StartAnimationTransition(const Animation& nextAnimation, float blendDuration, bool loop)
{


    // 現在表示中の姿勢を保存
    blendStartPose_.resize(
        skeleton_.joints.size()
    );

    for (
        size_t i = 0;
        i < skeleton_.joints.size();
        ++i
        ) {
        blendStartPose_[i] =
            skeleton_.joints[i].transform;
    }

    animation_ = nextAnimation;
    animationTime_ = 0.0f;

    blendTime_ = 0.0f;

    blendDuration_ =
        (std::max)(
            blendDuration,
            0.001f
            );

    isBlending_ = true;
    animationPlaying_ = true;
    animationLoop_ = loop;


}

void AnimationDebugController::UpdatePlayerMovement(Input* input)
{


    if (!input || !animatedObject_) {
        return;
    }

    constexpr float deltaTime = 1.0f / 60.0f;
    constexpr float pi = 3.14159265358979323846f;
    constexpr float twoPi = pi * 2.0f;

    // Slide中はWASD移動を受け付けない
    if (
        playerAnimationState_ ==
        PlayerAnimationState::Slide
        ) {
        return;
    }

    Vector3 moveDirection = {
        0.0f,
        0.0f,
        0.0f
    };

    if (input->PushKey(DIK_W)) {
        moveDirection.z += 1.0f;
    }

    if (input->PushKey(DIK_S)) {
        moveDirection.z -= 1.0f;
    }

    if (input->PushKey(DIK_A)) {
        moveDirection.x -= 1.0f;
    }

    if (input->PushKey(DIK_D)) {
        moveDirection.x += 1.0f;
    }

    const float length =
        std::sqrt(
            moveDirection.x * moveDirection.x +
            moveDirection.z * moveDirection.z
        );

    // 入力がなければ移動・回転しない
    if (length <= 0.0001f) {
        return;
    }

    // 斜め移動時も速度を一定にする
    moveDirection.x /= length;
    moveDirection.z /= length;

    // 座標を移動
    playerPosition_.x +=
        moveDirection.x *
        playerMoveSpeed_ *
        deltaTime;

    playerPosition_.z +=
        moveDirection.z *
        playerMoveSpeed_ *
        deltaTime;

    // 移動方向から目標Y回転を計算
    const float targetYaw =
        std::atan2(
            moveDirection.x,
            moveDirection.z
        );

    // -π～πの範囲で最短回転量を求める
    float yawDifference =
        targetYaw -
        playerFacingYaw_;

    yawDifference =
        std::remainder(
            yawDifference,
            twoPi
        );

    // 回転を滑らかに補間
    const float rotationBlend =
        (std::min)(
            playerTurnSpeed_ * deltaTime,
            1.0f
            );

    playerFacingYaw_ +=
        yawDifference *
        rotationBlend;

    animatedObject_->SetPosition(
        playerPosition_
    );

    // X回転は現在のモデル補正値を維持
    animatedObject_->SetRotation({
        -90.0f,
        playerFacingYaw_,
        0.0f
        });


}

const char*
AnimationDebugController::GetPlayerAnimationStateName() const
{
    switch (playerAnimationState_) {
    case PlayerAnimationState::Idle:
        return "Idle";

    case PlayerAnimationState::Walk:
        return "Walk";

    case PlayerAnimationState::Run:
        return "Run";

    case PlayerAnimationState::Slide:
        return "Slide";
    }

    return "Unknown";
}


void AnimationDebugController::ForceIdleAnimation()
{
    ChangePlayerAnimation(
        PlayerAnimationState::Idle
    );
}

void AnimationDebugController::ForceWalkAnimation()
{
    ChangePlayerAnimation(
        PlayerAnimationState::Walk
    );
}

void AnimationDebugController::ForceRunAnimation()
{
    ChangePlayerAnimation(
        PlayerAnimationState::Run
    );
}

void AnimationDebugController::ForceSlideAnimation()
{
    ChangePlayerAnimation(
        PlayerAnimationState::Slide
    );
}


void AnimationDebugController::ForceJumpAnimation()
{
    ChangePlayerAnimation(
        PlayerAnimationState::Jump
    );
}

void AnimationDebugController::UpdateSkeletonDebug() {
    if (skeletonDebugObjects_.size() != skeleton_.joints.size()) {
        return;
    }

    for (size_t i = 0; i < skeleton_.joints.size(); ++i) {
        const Matrix4x4& mat =
            skeleton_.joints[i].skeletonSpaceMatrix;

        Vector3 jointPosition = {
            mat.m[3][0],
            mat.m[3][1],
            mat.m[3][2]
        };

        skeletonDebugObjects_[i]->SetPosition(jointPosition);
        skeletonDebugObjects_[i]->Update();
    }

    size_t boneIndex = 0;

    for (const Joint& joint : skeleton_.joints) {
        if (!joint.parent) {
            continue;
        }

        if (boneIndex >= skeletonBoneObjects_.size()) {
            break;
        }

        const Matrix4x4& childMat =
            joint.skeletonSpaceMatrix;

        const Matrix4x4& parentMat =
            skeleton_.joints[*joint.parent].skeletonSpaceMatrix;

        Vector3 childPos = {
            childMat.m[3][0],
            childMat.m[3][1],
            childMat.m[3][2]
        };

        Vector3 parentPos = {
            parentMat.m[3][0],
            parentMat.m[3][1],
            parentMat.m[3][2]
        };

        Vector3 center = {
            (childPos.x + parentPos.x) * 0.5f,
            (childPos.y + parentPos.y) * 0.5f,
            (childPos.z + parentPos.z) * 0.5f
        };

        skeletonBoneObjects_[boneIndex]->SetPosition(center);
        skeletonBoneObjects_[boneIndex]->SetScale({ 0.15f, 0.15f, 0.15f });
        skeletonBoneObjects_[boneIndex]->Update();

        ++boneIndex;
    }
}

void AnimationDebugController::Draw() {
    if (animatedObject_) {
        animatedObject_->Draw();
    }

    if (!showSkeletonDebug_) {
        return;
    }

    for (auto& debugObject : skeletonDebugObjects_) {
        debugObject->Draw();
    }

    for (auto& boneObject : skeletonBoneObjects_) {
        boneObject->Draw();
    }
}

int AnimationDebugController::GetJointCount() const {
    return static_cast<int>(skeleton_.joints.size());
}

void AnimationDebugController::SetEnvironmentCoefficient(
    float environmentCoefficient
) {
    if (animatedObject_) {
        animatedObject_->SetEnvironmentCoefficient(environmentCoefficient);
    }

    for (auto& object : skeletonDebugObjects_) {
        object->SetEnvironmentCoefficient(environmentCoefficient);
    }

    for (auto& object : skeletonBoneObjects_) {
        object->SetEnvironmentCoefficient(environmentCoefficient);
    }
}

void AnimationDebugController::DrawImGui() {
#ifdef USE_IMGUI
    ImGui::Text("Animation Control");
    ImGui::Separator();

    if (ImGui::Button(
        animationPlaying_ ? "Pause" : "Play",
        ImVec2(100.0f, 28.0f)
    )) {
        animationPlaying_ = !animationPlaying_;
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset", ImVec2(100.0f, 28.0f))) {
        animationTime_ = 0.0f;

        ApplyAnimation(
            skeleton_,
            animation_,
            animationTime_
        );

        UpdateSkelton(skeleton_);
    }

    ImGui::Checkbox("Loop", &animationLoop_);
    ImGui::Checkbox("Auto Pause On Joint Edit", &autoPauseOnJointEdit_);
    ImGui::Checkbox("Show Skeleton Debug", &showSkeletonDebug_);

    ImGui::DragFloat(
        "Speed",
        &animationSpeed_,
        0.01f,
        0.0f,
        5.0f
    );

    if (animation_.duration > 0.0f) {
        ImGui::SliderFloat(
            "Time",
            &animationTime_,
            0.0f,
            animation_.duration
        );

        if (!animationPlaying_) {
            ApplyAnimation(
                skeleton_,
                animation_,
                animationTime_
            );

            UpdateSkelton(skeleton_);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Skeleton");

    const int jointCount =
        static_cast<int>(skeleton_.joints.size());

    ImGui::Text("Joint Count : %d", jointCount);

    if (jointCount == 0) {
        return;
    }

    selectedJointIndex_ =
        std::clamp(
            selectedJointIndex_,
            0,
            jointCount - 1
        );

    const char* previewName =
        skeleton_.joints[selectedJointIndex_].name.c_str();

    if (ImGui::BeginCombo("Selected Joint", previewName)) {
        for (int i = 0; i < jointCount; ++i) {
            const bool isSelected =
                selectedJointIndex_ == i;

            std::string label =
                std::to_string(i) +
                " : " +
                skeleton_.joints[i].name;

            if (ImGui::Selectable(label.c_str(), isSelected)) {
                selectedJointIndex_ = i;
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    Joint& joint =
        skeleton_.joints[selectedJointIndex_];

    ImGui::Spacing();
    ImGui::Text("Joint Detail");
    ImGui::Separator();

    ImGui::Text("Name   : %s", joint.name.c_str());
    ImGui::Text("Index  : %d", joint.index);

    if (joint.parent) {
        ImGui::Text("Parent : %d", *joint.parent);
    } else {
        ImGui::Text("Parent : none");
    }

    Vector3 translate = joint.transform.translate;
    Vector3 scale = joint.transform.scale;
    Quaternion rotate = joint.transform.rotate;

    bool edited = false;

    edited |= ImGui::DragFloat3(
        "Local Translate",
        &translate.x,
        0.01f,
        -100.0f,
        100.0f
    );

    edited |= ImGui::DragFloat3(
        "Local Scale",
        &scale.x,
        0.01f,
        0.001f,
        100.0f
    );

    edited |= ImGui::DragFloat4(
        "Local Rotate Quaternion",
        &rotate.x,
        0.01f,
        -1.0f,
        1.0f
    );

    if (ImGui::Button(
        "Normalize Quaternion",
        ImVec2(180.0f, 28.0f)
    )) {
        rotate = Math::Normalize(rotate);
        edited = true;
    }

    if (edited) {
        if (autoPauseOnJointEdit_) {
            animationPlaying_ = false;
        }

        joint.transform.translate = translate;
        joint.transform.scale = scale;
        joint.transform.rotate = Math::Normalize(rotate);

        UpdateSkelton(skeleton_);
    }

    const Matrix4x4& skeletonMatrix =
        joint.skeletonSpaceMatrix;

    Vector3 skeletonPosition = {
        skeletonMatrix.m[3][0],
        skeletonMatrix.m[3][1],
        skeletonMatrix.m[3][2]
    };

    ImGui::Spacing();
    ImGui::Text("Skeleton Space");
    ImGui::BulletText(
        "Position : %.3f, %.3f, %.3f",
        skeletonPosition.x,
        skeletonPosition.y,
        skeletonPosition.z
    );

    if (ImGui::TreeNode("Children")) {
        for (int32_t childIndex : joint.children) {
            const Joint& child =
                skeleton_.joints[childIndex];

            ImGui::BulletText(
                "%d : %s",
                childIndex,
                child.name.c_str()
            );
        }

        ImGui::TreePop();
    }
#endif
}