#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "MyMath.h"
#include "DirectXCommon.h"
#include "TextureManager.h"
#include<list>

class Ring
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

    struct RingParticle {
        Transform transform;
        Vector4 color;
        float lifeTime;
        float maxTime;
    };

    struct Settings
    {
        // Ringの基本色
        Vector4 color = {
            0.35f,
            0.70f,
            1.00f,
            1.00f
        };

        // 発光強度
        float intensity = 1.0f;

        // 発生時の大きさ
        float startScale = 0.45f;

        // 消える直前の大きさ
        float endScale = 1.65f;

        // Ringの太さ
        // 0.02で細く、0.8で太くなる
        float thickness = 0.18f;

        // 表示時間
        float lifeTime = 0.45f;

        // 寿命のうち何割をフェードインに使うか
        float fadeInRatio = 0.1f;

        // 1.0で直線、数値が大きいほど最初に速く広がる
        float easePower = 3.0f;

        // Z軸回転速度
        float rotationSpeed = 0.0f;
    };

    Settings& GetSettings()
    {
        return settings_;
    }

    const Settings& GetSettings() const
    {
        return settings_;
    }

    void SetThickness(float thickness);

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

    static const uint32_t kMaxRings = 64;

    std::list<RingParticle> rings_;


    // Ringの大きさ
    float startScale_ = 0.45f;
    float endScale_ = 1.65f;

    // 最初の不透明度
    float startAlpha_ = 0.75f;

    Settings settings_{};

    // 厚さ変更時にメッシュを作り直す
    bool meshDirty_ = false;
};

