#pragma once
#include "MyMath.h"
#include <wrl.h>
#include <d3d12.h>

class SpriteCommon;

class Sprite {
public:
    void Initialize(SpriteCommon* spriteCommon);
    void Update();
    void Draw();

    struct VertexData {
        MyMath::Vector4 position;
        MyMath::Vector2 texcoord;
        MyMath::Vector3 normal;
    };

    struct Material {
        MyMath::Vector4 color;
        int32_t enableLighting;
        float padding[3];
        MyMath::Matrix4x4 uvTransform;
    };

    struct TransformationMatrix {
        MyMath::Matrix4x4 WVP;
        MyMath::Matrix4x4 World;
    };

private:
    SpriteCommon* spriteCommon_ = nullptr;

    // ===== Vertex
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    VertexData* vertexData_ = nullptr;

    // ===== Index
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    uint32_t* indexData_ = nullptr;

    // ===== Material
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;

    // ===== TransformationMatrix
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    TransformationMatrix* transformationMatrixData_ = nullptr;

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};

    MyMath::Vector2 position = { 0.0f,0.0f };

    const MyMath::Vector2& GetPosition()const { return position; }
    void SetPosition(const MyMath::Vector2& position) { this->position = position; }

    float rotation = 0.0f;
    float GetRotation()const { return rotation; }
    void SetRotation(float rotation) { this->rotation = rotation; }

private:
    void CreateVertexResource();
    void CreateIndexResource();
    void CreateVertexBufferView();
    void CreateIndexBufferView();
    void CreateMaterialResource();
    void CreateTransformationMatrixResource();

    void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);
};