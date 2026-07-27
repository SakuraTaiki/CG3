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

void Object3dSkinning::DispatchComputeSkinning() {
    if (!object3dCommon_ || !hasSkinCluster_ ||
        skinCluster_.vertexCount == 0 ||
        !skinCluster_.computeDispatchRequired) {
        return;
    }

    ID3D12GraphicsCommandList* commandList =
        object3dCommon_->GetDxCommon()->GetCommandList();

    if (skinCluster_.skinnedVertexIsReadyForDraw) {
        D3D12_RESOURCE_BARRIER toUav{};
        toUav.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toUav.Transition.pResource =
            skinCluster_.skinnedVertexResource.Get();
        toUav.Transition.StateBefore =
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        toUav.Transition.StateAfter =
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        toUav.Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &toUav);
    }

    commandList->SetComputeRootSignature(
        object3dCommon_->GetSkinningComputeRootSignature()
    );
    commandList->SetPipelineState(
        object3dCommon_->GetSkinningComputePipelineState()
    );
    commandList->SetComputeRootDescriptorTable(
        0,
        skinCluster_.sourceVertexSrvHandle
    );
    commandList->SetComputeRootDescriptorTable(
        1,
        skinCluster_.influenceSrvHandle
    );
    commandList->SetComputeRootDescriptorTable(
        2,
        skinCluster_.paletteSrvHandle
    );
    commandList->SetComputeRootDescriptorTable(
        3,
        skinCluster_.skinnedVertexUavHandle
    );
    commandList->SetComputeRoot32BitConstant(
        4,
        skinCluster_.vertexCount,
        0
    );
    commandList->Dispatch(
        (skinCluster_.vertexCount + 1023u) / 1024u,
        1,
        1
    );

    D3D12_RESOURCE_BARRIER uavBarrier{};
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource =
        skinCluster_.skinnedVertexResource.Get();
    commandList->ResourceBarrier(1, &uavBarrier);

    D3D12_RESOURCE_BARRIER toVertexBuffer{};
    toVertexBuffer.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toVertexBuffer.Transition.pResource =
        skinCluster_.skinnedVertexResource.Get();
    toVertexBuffer.Transition.StateBefore =
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    toVertexBuffer.Transition.StateAfter =
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    toVertexBuffer.Transition.Subresource =
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &toVertexBuffer);
    skinCluster_.skinnedVertexIsReadyForDraw = true;
    skinCluster_.computeDispatchRequired = false;
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
