#include "Object3d.h"
#include "TextureManager.h" // GetSrvHandleGPUを使うために必要
#include <cassert>
#include <cmath>

void Object3d::Initialize(Object3dCommon* object3dCommon) {
    assert(object3dCommon);
    object3dCommon_ = object3dCommon;
    auto device = object3dCommon_->GetDxCommon()->GetDevice();

    camera_ = object3dCommon_->GetDefaultCamera();

    // Transform Buffer
    D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = (sizeof(TransformationMatrix) + 0xff) & ~0xff;
    resDesc.Height = 1; resDesc.DepthOrArraySize = 1; resDesc.MipLevels = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; resDesc.SampleDesc.Count = 1;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&transformationResource_));
    transformationResource_->Map(0, nullptr, (void**)&transformationData_);
    transformationData_->WVP = Math::MakeIdentity4x4();
    transformationData_->World = Math::MakeIdentity4x4();

    // Material Buffer
    resDesc.Width = (sizeof(Material) + 0xff) & ~0xff;
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&materialResource_));
    materialResource_->Map(0, nullptr, (void**)&materialData_);

    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = 1;
    materialData_->environmentCoefficient = 0.05f;
    materialData_->uvTransform = Math::MakeIdentity4x4();

    // Camera Buffer
    resDesc.Width = (sizeof(CameraForGPU) + 0xff) & ~0xff;

    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&cameraResource_)
    );

    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));
    cameraData_->worldPosition = { 0.0f, 0.0f, 0.0f };
    cameraData_->padding = 0.0f;
}

void Object3d::Update() {
    Matrix4x4 worldMatrix = Math::MakeAffineMatrix(
        transform_.scale,
        transform_.rotate, 
        transform_.translate);

    Matrix4x4 worldViewProjectionmatrix = worldMatrix;


    if (camera_) {
        worldViewProjectionmatrix =
            Math::Multiply(worldMatrix, camera_->GetViewProjectionMatrix());

        if (cameraData_) {
            cameraData_->worldPosition = camera_->GetTranslate();
        }
    }

    if (model_) {
        Matrix4x4 localMatrix = model_->GetRootNode().localMatrix;

        if (useAnimation_) {
            animationTime_ += 1.0f / 60.0f;

            if (animation_.duration > 0.0f) {
                animationTime_ = std::fmod(animationTime_, animation_.duration);
            }

            auto it = animation_.nodeAnimations.find(model_->GetRootNode().name);

            if (it != animation_.nodeAnimations.end()) {
                const NodeAnimation& rootNodeAnimation = it->second;

                Vector3 translate =
                    CalculateValue(rootNodeAnimation.translate, animationTime_);

                Quaternion rotate =
                    CalculateValue(rootNodeAnimation.rotate, animationTime_);

                Vector3 scale =
                    CalculateValue(rootNodeAnimation.scale, animationTime_);

                localMatrix =
                    Math::MakeAffineMatrix(scale, rotate, translate);
            }
        }

        Matrix4x4 worldWithRoot = Math::Multiply(localMatrix, worldMatrix);
        Matrix4x4 wvpWithRoot = worldWithRoot;

        if (camera_) {
            wvpWithRoot =
                Math::Multiply(worldWithRoot, camera_->GetViewProjectionMatrix());
        }

        transformationData_->WVP = wvpWithRoot;
        transformationData_->World = worldWithRoot;
    } else {
        transformationData_->WVP = worldViewProjectionmatrix;
        transformationData_->World = worldMatrix;
    }
}

void Object3d::SetUVTransform(const Transform& t) {
    if (materialData_) {
        Matrix4x4 w = Math::MakeAffineMatrix(t.scale, t.rotate, t.translate);
        materialData_->uvTransform = w;
    }
}

void Object3d::Draw() {
    if (!model_) {
        return;
    }

    auto commandList = object3dCommon_->GetDxCommon()->GetCommandList();

    // 0. Material : PS b0
    commandList->SetGraphicsRootConstantBufferView(
        0,
        materialResource_->GetGPUVirtualAddress()
    );

    // 1. Transform : VS b0
    commandList->SetGraphicsRootConstantBufferView(
        1,
        transformationResource_->GetGPUVirtualAddress()
    );

    // 2. Light : PS b1
    commandList->SetGraphicsRootConstantBufferView(
        2,
        object3dCommon_->GetLightGPUVirtualAddress()
    );

    // 3. Camera : PS b2
    commandList->SetGraphicsRootConstantBufferView(
        3,
        cameraResource_->GetGPUVirtualAddress()
    );

    if (object3dCommon_->GetTextureManager()) {
        // 4. Texture2D : PS t0
        auto gpuHandle =
            object3dCommon_->GetTextureManager()->GetSrvHandleGPU(
                model_->GetTextureHandle()
            );

        commandList->SetGraphicsRootDescriptorTable(
            4,
            gpuHandle
        );

        // 5. Environment TextureCube : PS t1
        if (environmentTextureHandle_ != 0) {
            auto envHandle =
                object3dCommon_->GetTextureManager()->GetSrvHandleGPU(
                    environmentTextureHandle_
                );

            commandList->SetGraphicsRootDescriptorTable(
                5,
                envHandle
            );
        } else {
            auto envHandle =
                object3dCommon_->GetTextureManager()->GetSrvHandleGPU(
                    model_->GetTextureHandle()
                );

            commandList->SetGraphicsRootDescriptorTable(
                5,
                envHandle
            );
        }
    }

    model_->Draw(commandList);
}