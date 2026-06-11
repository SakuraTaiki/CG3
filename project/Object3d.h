#pragma once
#include "Object3dCommon.h"
#include "Model.h"
#include "MyMath.h"
#include"Camera.h"
#include "Animation.h"

struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
};

struct CameraForGPU {
    Vector3 worldPosition;
    float padding;
};

struct Material {
    Vector4 color;
    int32_t enableLighting;

    float environmentCoefficient;
    float padding[2];

    Matrix4x4 uvTransform;
};

class Object3d {
public:
    void Initialize(Object3dCommon* object3dCommon);
    void Update();
    void Draw();

    void SetModel(Model* model) { model_ = model; }
    void SetPosition(const Vector3& position) { transform_.translate = position; }
    void SetRotation(const Vector3& rotation) { transform_.rotate = rotation; }
    void SetScale(const Vector3& scale) { transform_.scale = scale; }

    // カメラ設定
    void SetCamera(Camera* camera) { camera_ = camera; }

    // マテリアル制御
    void SetColor(const Vector4& color) { if (materialData_) materialData_->color = color; }
    void SetEnableLighting(bool enable) { if (materialData_) materialData_->enableLighting = (enable ? 1 : 0); }
    void SetUVTransform(const Transform& uvTransform);

    void SetEnvironmentCoefficient(float value) {
        if (materialData_) {
            materialData_->environmentCoefficient = value;
        }
    }

    void SetEnvironmentTexture(uint32_t textureHandle) {
        environmentTextureHandle_ = textureHandle;
    }

    float GetEnvironmentCoefficient() const {
        if (!materialData_) {
            return 0.0f;
        }

        return materialData_->environmentCoefficient;
    }

    //アニメーションセット
    void SetAnimation(const Animation& animation) {
        animation_ = animation;
        useAnimation_ = true;
        animationTime_ = 0.0f;
    }

private:
    Object3dCommon* object3dCommon_ = nullptr;
    Model* model_ = nullptr;

    Transform transform_ = { {1,1,1}, {0,0,0}, {0,0,0} };
    
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource_;
    TransformationMatrix* transformationData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;

    //カメラ
    Camera* camera_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
    CameraForGPU* cameraData_ = nullptr;

    uint32_t environmentTextureHandle_ = 0;

    //アニメーションメンバ
    Animation animation_;
    bool useAnimation_ = false;
    float animationTime_ = 0.0f;
};