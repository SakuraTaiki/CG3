#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <list>

#include "MyMath.h"
#include "DirectXCommon.h"
#include "TextureManager.h"

class Cylinder
{

public:
    struct VertexData {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    struct InstanceData {
        Matrix4x4 WVP;
        Vector4 color;
    };

    struct CylinderParticle {
        Transform transform;
        Vector4 color;
        float lifeTime;
        float maxTime;
    };

    void Initialize(DirectXCommon* dxCommon, TextureManager* textureManager);
    void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);
    void Draw();

    void SetIsActive(bool isActive) { isActive_ = isActive; }
    bool GetIsActive() const { return isActive_; }

    void Emit(const Vector3& position);

private:
    void CreateRootSignature();
    void CreatePipelineState();
    void CreateMesh();

private:
    static const uint32_t kMaxCylinders = 64;

    DirectXCommon* dxCommon_ = nullptr;
    TextureManager* textureManager_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    uint32_t vertexCount_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> instancingBuffer_;
    D3D12_VERTEX_BUFFER_VIEW instancingBufferView_{};
    InstanceData* instancingDataMapped_ = nullptr;

    uint32_t textureHandle_ = 0;

    bool isActive_ = true;

    std::list<CylinderParticle> cylinders_;

    float maxHeight_ = 2.5f;

};

