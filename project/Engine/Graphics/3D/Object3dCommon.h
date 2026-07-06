#pragma once
#include "DirectXCommon.h"
#include <d3d12.h>
#include <wrl.h>
#include "MyMath.h"
#include"Camera.h"
#include "SrvManager.h"

// 前方宣言
class TextureManager;

// 共通のライト構造体
struct DirectionalLight {
    Vector4 color;
    Vector3 direction;
    float intensity;
};

struct SpotLight
{
    Vector4 color;          // ライト色
    Vector3 position;       // ライト位置
    float intensity;        // 輝度

    Vector3 direction;      // ライトが照らす方向
    float distance;         // 最大到達距離

    float decay;            // 距離減衰率
    float cosAngle;         // 外側コーン角度のcos
    float cosFalloffStart;  // 減衰開始角度のcos
    float padding;
};

class Object3dCommon {
public:
    void Initialize(DirectXCommon* dxCommon);
    void PreDraw(); // 描画前設定

    DirectXCommon* GetDxCommon() const { return dxCommon_; }
    ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
    ID3D12PipelineState* GetPipelineState() const { return pipelineState_.Get(); }

    // ライト制御
    void SetDefaultLight();
    void SetLightDirection(const Vector3& direction) { if (lightData_) lightData_->direction = Math::Normalize(direction); }
    void SetLightColor(const Vector4& color) { if (lightData_) lightData_->color = color; }
    void SetLightIntensity(float intensity) { if (lightData_) lightData_->intensity = intensity; }

    //====================================
    //SpotLightSet
    //====================================

    void SetSpotLightColor(const Vector4& color) {
        if (spotLightData_) {
            spotLightData_->color = color;
        }
    }

    void SetSpotLightPosition(const Vector3& position) {
        if (spotLightData_) {
            spotLightData_->position = position;
        }
    }

    void SetSpotLightIntensity(float intensity) {
        if (spotLightData_) {
            spotLightData_->intensity = intensity;
        }
    }

    void SetSpotLightDirection(const Vector3& direction) {
        if (spotLightData_) {
            spotLightData_->direction = Math::Normalize(direction);
        }
    }

    void SetSpotLightDistance(float distance) {
        if (spotLightData_) {
            spotLightData_->distance = distance;
        }
    }

    void SetSpotLightDecay(float decay) {
        if (spotLightData_) {
            spotLightData_->decay = decay;
        }
    }

    void SetSpotLightCosAngle(float cosAngle) {
        if (spotLightData_) {
            spotLightData_->cosAngle = cosAngle;
        }
    }

    void SetSpotLightCosFalloffStart(float cosFalloffStart) {
        if (spotLightData_) {
            spotLightData_->cosFalloffStart = cosFalloffStart;
        }
    }


    //=============================================
    //Imgui用LightGet関数群
    //=============================================

    DirectionalLight& GetDirectionalLight() {
        return *lightData_;
    }

    const DirectionalLight& GetDirectionalLight() const {
        return *lightData_;
    }

    SpotLight& GetSpotLight() {
        return *spotLightData_;
    }

    const SpotLight& GetSpotLight() const {
        return *spotLightData_;
    }



    D3D12_GPU_VIRTUAL_ADDRESS GetSpotLightGPUVirtualAddress() const {
        return spotLightResource_->GetGPUVirtualAddress();
    }

    D3D12_GPU_VIRTUAL_ADDRESS GetLightGPUVirtualAddress() const { return lightResource_->GetGPUVirtualAddress(); }

    // ★重要: TextureManagerのセット
    void SetTextureManager(TextureManager* textureManager) { textureManager_ = textureManager; }
    TextureManager* GetTextureManager() const { return textureManager_; }

    //デフォルトカメラのセット
    void SetDefaultCamera(Camera* camera) { defaultCamera_ = camera; }
    Camera* GetDefaultCamera() const { return defaultCamera_; }

    void SetSrvManager(SrvManager* srvManager) { srvManager_ = srvManager; }
    SrvManager* GetSrvManager() const { return srvManager_; }

private:
    void CreateRootSignature();
    void CreateGraphicsPipeline();
    void CreateLightBuffer();
    void CreateSpotLightBuffer();

private:
    DirectXCommon* dxCommon_ = nullptr;
    TextureManager* textureManager_ = nullptr; // テクスチャ管理クラスへのポインタ

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

    // 平行光源用
    Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_;
    DirectionalLight* lightData_ = nullptr;
    SpotLight* spotLightData_ = nullptr;

    //デフォルトカメラ用メンバ変数
    Camera* defaultCamera_ = nullptr;

    SrvManager* srvManager_ = nullptr;


};