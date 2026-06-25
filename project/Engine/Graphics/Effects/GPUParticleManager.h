#pragma once

#include <cstdint>
#include <d3d12.h>
#include <vector>
#include <wrl.h>

#include "DirectXCommon.h"
#include "MyMath.h"
#include "SrvManager.h"
#include "TextureManager.h"

class GPUParticleManager
{
public:
    static const uint32_t kMaxParticles = 1024;
    static const uint32_t kThreadCount = 1024;

    void Initialize(
        DirectXCommon* dxCommon,
        SrvManager* srvManager,
        TextureManager* textureManager
    );

    void Update(
        const Matrix4x4& viewMatrix,
        const Matrix4x4& projectionMatrix
    );

    void Draw();

    void Emit(
        const Vector3& position,
        uint32_t count,
        float sizeMultiplier = 1.0f
    );

    void EmitSakura(
        const Vector3& position,
        uint32_t count,
        float sizeMultiplier = 1.0f
    );

private:
    struct ParticleData
    {
        Vector3 translate;
        float lifeTime;
        Vector3 scale;
        float maxTime;
        Vector3 startScale;
        float angularVelocity;
        Vector3 velocity;
        float effectType;
        Vector3 acceleration;
        float isAlive;
        Vector4 color;
        float rotateZ;
        float pad[3];
    };

    struct VertexData
    {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    struct ViewProjectionData
    {
        Matrix4x4 billboard;
        Matrix4x4 viewProjection;
    };

    struct UpdateData
    {
        float deltaTime;
        float totalTime;
        uint32_t particleCount;
        float pad;
    };

    void CreateBuffers();
    void CreateDescriptors();
    void CreateGraphicsRootSignature();
    void CreateGraphicsPipelineState();
    void CreateComputeRootSignature();
    void CreateComputePipelineState();
    void CreateMesh();
    void InitializeParticlesOnGPU();
    void UploadPendingParticles();
    void TransitionParticleResource(D3D12_RESOURCE_STATES afterState);
   

private:
    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;
    TextureManager* textureManager_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> graphicsRootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> computeRootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> computePipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> initializePipelineState_;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> particleBuffer_;
    D3D12_RESOURCE_STATES particleResourceState_ = D3D12_RESOURCE_STATE_COPY_DEST;

    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer_;
    ParticleData* uploadData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> viewProjectionBuffer_;
    ViewProjectionData* viewProjectionData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> updateBuffer_;
    UpdateData* updateData_ = nullptr;

    uint32_t particleSrvIndex_ = 0;
    uint32_t particleUavIndex_ = 0;
    uint32_t textureHandle_ = 0;
    uint32_t emitIndex_ = 0;
    float totalTime_ = 0.0f;

    std::vector<uint32_t> pendingIndices_;
};
