#pragma once

#include <cstdint>
#include <d3d12.h>
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

    struct EmitterSphere
    {
        Vector3 translate;
        float radius;
        uint32_t count;
        float frequency;
        float frequencyTime;
        uint32_t emit;
        float effectType;
        float sizeMultiplier;
        float pad[2];
    };

    struct PerFrame
    {
        float time;
        float deltaTime;
        float pad[2];
    };

    void CreateBuffers();
    void CreateDescriptors();
    void CreateGraphicsRootSignature();
    void CreateGraphicsPipelineState();
    void CreateComputeRootSignature();
    void CreateComputePipelineState();
    void CreateMesh();
    void InitializeParticlesOnGPU();
    void TransitionParticleResource(D3D12_RESOURCE_STATES afterState);
    void DispatchEmit();
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


    Microsoft::WRL::ComPtr<ID3D12Resource> viewProjectionBuffer_;
    ViewProjectionData* viewProjectionData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> updateBuffer_;
    UpdateData* updateData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> emitPipelineState_;

    Microsoft::WRL::ComPtr<ID3D12Resource> freeCounterBuffer_;

    Microsoft::WRL::ComPtr<ID3D12Resource> emitterBuffer_;
    EmitterSphere* emitterData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> perFrameBuffer_;
    PerFrame* perFrameData_ = nullptr;

    uint32_t particleSrvIndex_ = 0;
    uint32_t particleUavIndex_ = 0;
    uint32_t textureHandle_ = 0;
    uint32_t freeCounterUavIndex_ = 0;
    bool emitRequested_ = false;
    float totalTime_ = 0.0f;

};
