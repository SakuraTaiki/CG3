#pragma once
#include "Math.h"

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <vector>

class Sprite;
class ModelCommon;
class Model;
class Object3dCommon;
class Object3d
{
public:
    void Initialize(Object3dCommon* object3dCommon, ModelCommon* modelCommon);
    void Update();
    void Draw();
private:
    Object3dCommon* object3dCommon_ = nullptr;
    Model* model = nullptr;

    // ===== 行列
    struct TransformationMatrix {
        Math::Matrix4x4 WVP;
        Math::Matrix4x4 World;
    };

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    TransformationMatrix* transformationMatrixData_ = nullptr;

    void CreateTransformationMatrix();

    // ===== ライト
    struct DirectionalLight {
        Math::Vector4 color;
        Math::Vector3 direction;
        float padding;
    };

    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    DirectionalLight* directionalLightData_ = nullptr;

    void CreateDirectionalLight();

    // ===== Transform
    Math::Transform transform;
    Math::Transform cameraTransform;

public:
    //setter
    void SetModel(Model* model) { this->model = model; }
};