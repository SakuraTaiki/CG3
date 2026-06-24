#include "AnimationDebugController.h"
#include "AnimationLoader.h"
#include "ModelManager.h"
#include "Object3dCommon.h"

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
        ModelManager::Load("Resources/human", "walk.gltf");

    animation_ =
        AnimationLoader::Load("Resources/human", "walk.gltf");

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
    animatedObject_->SetRotation({ 0.0f, 0.0f, 0.0f });
    animatedObject_->SetScale({ 1.0f, 1.0f, 1.0f });

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
        jointObj->SetScale({ 0.3f, 0.3f, 0.3f });
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
        boneObj->SetScale({ 0.1f, 0.1f, 0.1f });
        boneObj->SetRotation({ 0.0f, 0.0f, 0.0f });
        boneObj->SetPosition({ 0.0f, 0.0f, 0.0f });
        boneObj->SetEnvironmentTexture(environmentTextureHandle);
        boneObj->SetEnvironmentCoefficient(environmentCoefficient);

        skeletonBoneObjects_.push_back(std::move(boneObj));
    }
}

void AnimationDebugController::Update() {
    UpdateAnimation();

    if (showSkeletonDebug_) {
        UpdateSkeletonDebug();
    }

    SyncSkeletonToObject();

    if (animatedObject_) {
        animatedObject_->Update();
    }
}

void AnimationDebugController::UpdateAnimation() {
    if (animationPlaying_) {
        animationTime_ +=
            (1.0f / 60.0f) * animationSpeed_;

        if (animation_.duration > 0.0f) {
            if (animationLoop_) {
                animationTime_ =
                    std::fmod(
                        animationTime_,
                        animation_.duration
                    );
            } else if (animationTime_ > animation_.duration) {
                animationTime_ = animation_.duration;
                animationPlaying_ = false;
            }
        }

        ApplyAnimation(
            skeleton_,
            animation_,
            animationTime_
        );
    }

    UpdateSkelton(skeleton_);
}

void AnimationDebugController::SyncSkeletonToObject() {
    if (!animatedObject_) {
        return;
    }

    animatedObject_->SetSkeleton(skeleton_);
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