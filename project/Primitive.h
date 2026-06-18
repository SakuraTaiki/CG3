#pragma once

#include <cstdint>
#include <d3d12.h>
#include <list>
#include <wrl.h>

#include "MyMath.h"
#include "DirectXCommon.h"
#include "TextureManager.h"

class Primitive
{
public:
    static const uint32_t kMaxParticles = 1024;

    struct Particle
    {
        Transform transform;
        Vector3 initialScale;
        Vector3 velocity;

        Vector4 color;

        float angularVelocity;
        float lifeTime;
        float maxTime;
    };

    struct InstanceData
    {
        Matrix4x4 WVP;
        Vector4 color;
    };

    struct VertexData
    {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };


    struct Settings
    {
        Vector4 color = {
            1.0f,
            0.85f,
            0.35f,
            1.0f
        };

        int count = 8;

        float intensity = 1.0f;

        float width = 0.05f;

        float minLength = 0.4f;
        float maxLength = 1.5f;

        float minLifeTime = 0.4f;
        float maxLifeTime = 0.8f;

        // 外側へ飛ぶ速度
        float moveSpeed = 0.0f;

        // 毎秒の回転速度
        float rotationSpeed = 0.0f;

        // 消えるときの横幅
        float endWidthScale = 1.0f;

        // 消えるときの長さ
        float endLengthScale = 1.0f;

        // 1.0で直線的、数値を上げると早く消える
        float fadePower = 1.0f;
    };

    Settings& GetSettings()
    {
        return settings_;
    }

    const Settings& GetSettings() const
    {
        return settings_;
    }

    // Settingsのcountを使用するEmit
    void Emit(const Vector3& position);

public:
    void Initialize(
        DirectXCommon* dxCommon,
        TextureManager* textureManager
    );

    void Update(
        const Matrix4x4& viewMatrix,
        const Matrix4x4& projectionMatrix
    );

    void Draw();

    void Emit(
        const Vector3& position,
        uint32_t count
    );

    void SetIsActive(bool isActive)
    {
        isActive_ = isActive;
    }

    bool GetIsActive() const
    {
        return isActive_;
    }

private:
    void CreateRootSignature();
    void CreatePipelineState();
    void CreateMesh();

private:
    DirectXCommon* dxCommon_ = nullptr;
    TextureManager* textureManager_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>
        rootSignature_;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>
        pipelineState_;

    Microsoft::WRL::ComPtr<ID3D12Resource>
        vertexBuffer_;

    D3D12_VERTEX_BUFFER_VIEW
        vertexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource>
        instancingBuffer_;

    D3D12_VERTEX_BUFFER_VIEW
        instancingBufferView_{};

    InstanceData* instancingDataMapped_ = nullptr;

    uint32_t textureHandle_ = 0;

    std::list<Particle> particles_;

    bool isActive_ = true;

    Settings settings_{};
};