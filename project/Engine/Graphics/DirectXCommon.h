#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <dxcapi.h>
#include <cstdint>
#include <string>

#include "DirectXShaderCompiler.h"
#include "FpsLimiter.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

class WinApp;

class DirectXCommon {
public: // サブクラス定義
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public: // メンバ関数
    DirectXCommon() = default;
    ~DirectXCommon() = default;

    // 初期化
    void Initialize(WinApp* winApp);

    // 描画前処理
    void PreDraw();
    // 描画後処理
    void PostDraw();

    // シェーダーコンパイル
    ComPtr<IDxcBlob> CompileShader(
        const std::wstring& filePath,
        const wchar_t* profile);

    // ゲッター
    ID3D12Device* GetDevice() const { return device_.Get(); }

    ID3D12CommandQueue* GetCommandQueue() const { return commandQueue_.Get(); }

    DXGI_FORMAT GetRTVFormat() const { return DXGI_FORMAT_R8G8B8A8_UNORM; }

    DXGI_FORMAT GetDSVFormat() const { return DXGI_FORMAT_D24_UNORM_S8_UINT; }

    size_t GetSwapChainResourcesNum() const { return 2; }

    ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
    size_t GetBackBufferCount() const { return 2; }

    void PreDrawForRenderTexture();
    void DrawRenderTextureToSwapChain();

    D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTextureSrvHandleCPU() const {
        return renderTextureSrvHandleCPU_;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetRenderTextureSrvHandleGPU() const {
        return renderTextureSrvHandleGPU_;
    }

    void CopyRenderTextureSrvTo(D3D12_CPU_DESCRIPTOR_HANDLE destinationHandle);


    void SetGrayScale(bool enable) {
        enableGrayScale_ = enable;
    }

    bool GetGrayScale() const {
        return enableGrayScale_;
    }

    
    struct VignetteSettings {
        bool enabled = false;
        float intensity = 0.75f;
        float radius = 0.75f;
        float softness = 0.35f;
    };

    VignetteSettings& GetVignetteSettings() {
        return vignetteSettings_;
    }


    struct SmoothingSettings {
        bool enabled = false;
        int radius = 1;
        float strength = 1.0f;
    };

    SmoothingSettings& GetSmoothingSettings() {
        return smoothingSettings_;
    }

    //==================
    //GaussianFilter
    //==================

    struct GaussianSettings {
        bool enabled = false;
        int radius = 2;
        float sigma = 2.0f;
        float strength = 1.0f;
    };

    GaussianSettings& GetGaussianSettings() {
        return gaussianSettings_;
    }

    const GaussianSettings& GetGaussianSettings() const {
        return gaussianSettings_;
    }


    struct OutlineSettings {
        bool enabled = true;

        float color[4] = {
            0.0f,
            0.0f,
            0.0f,
            1.0f
        };

        // 小さいほど弱い深度差も線になる
        float threshold = 0.05f;

        float strength = 1.0f;

        // 1～4
        int thickness = 1;

        // Cameraと同じ値にする
        float nearClip = 0.1f;
        float farClip = 100.0f;
    };

    OutlineSettings& GetOutlineSettings() {
        return outlineSettings_;
    }

    const OutlineSettings& GetOutlineSettings() const {
        return outlineSettings_;
    }


    struct RadialBlurSettings {
        bool enabled = false;

        // UV座標。画面中央は(0.5, 0.5)
        float center[2] = {
            0.5f,
            0.5f
        };

        float blurWidth = 0.01f;
        float strength = 1.0f;
        int sampleCount = 10;
    };

    RadialBlurSettings& GetRadialBlurSettings() {
        return radialBlurSettings_;
    }

    const RadialBlurSettings&
        GetRadialBlurSettings() const {
        return radialBlurSettings_;
    }


    struct DissolveSettings {
        bool enabled = false;

        float threshold = 0.0f;
        float edgeWidth = 0.03f;
        float edgeIntensity = 1.0f;

        float edgeColor[4] = {
            1.0f,
            0.4f,
            0.0f,
            1.0f
        };
    };

    DissolveSettings& GetDissolveSettings() {
        return dissolveSettings_;
    }

    const DissolveSettings&
        GetDissolveSettings() const {
        return dissolveSettings_;
    }

    void SetDissolveMaskSrv(
        D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle
    );

    //================================
    //Random
    //================================

    struct RandomSettings {
        bool enabled = false;
        bool animate = true;
        bool showNoiseOnly = false;

        float speed = 1.0f;
        float scale = 720.0f;
        float strength = 1.0f;
    };

    RandomSettings& GetRandomSettings() {
        return randomSettings_;
    }

    const RandomSettings&
        GetRandomSettings() const {
        return randomSettings_;
    }

    void ResetRandomTime() {
        randomTime_ = 0.0f;
    }

    void PrepareRenderTextureForImgui();

private: // メンバ関数(内部処理)
    void InitializeDevice();
    void InitializeCommand();
    void InitializeSwapChain();
    void InitializeRenderTargetView();
    void InitializeDepthStencilView();
    void InitializeFence();
  

    void InitializeRenderTexture();
    void InitializeCopyImagePipeline();
    void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

    void ResizeIfNeeded();
    void WaitForGPU();

private: // メンバ変数
    WinApp* winApp_ = nullptr;

    // DirectX主要オブジェクト
    ComPtr<IDXGIFactory7> dxgiFactory_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> commandQueue_;
    ComPtr<ID3D12CommandAllocator> commandAllocator_;
    ComPtr<ID3D12GraphicsCommandList> commandList_;
    ComPtr<IDXGISwapChain4> swapChain_;

    // RTV (レンダーターゲットビュー)
    ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
    ComPtr<ID3D12Resource> swapChainResources_[2];

    // DSV (深度ステンシルビュー)
    ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
    ComPtr<ID3D12Resource> depthStencilResource_;

    // フェンス (同期用)
    ComPtr<ID3D12Fence> fence_;
    uint64_t fenceValue_ = 0;
    HANDLE fenceEvent_ = nullptr;

    // HLSL コンパイル担当。
    DirectXShaderCompiler shaderCompiler_;

    // 60FPS 固定担当。
    FpsLimiter fpsLimiter_;

    ComPtr<ID3D12Resource> renderTextureResource_;

    ComPtr<ID3D12DescriptorHeap> renderTextureSrvHeap_;
    D3D12_CPU_DESCRIPTOR_HANDLE renderTextureSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE renderTextureSrvHandleGPU_{};

    D3D12_CPU_DESCRIPTOR_HANDLE renderTextureRtvHandle_{};

    D3D12_CPU_DESCRIPTOR_HANDLE depthTextureSrvHandleCPU_{};

    D3D12_GPU_DESCRIPTOR_HANDLE depthTextureSrvHandleGPU_{};

    D3D12_CPU_DESCRIPTOR_HANDLE dissolveMaskSrvHandleCPU_{};

    D3D12_GPU_DESCRIPTOR_HANDLE dissolveMaskSrvHandleGPU_{};


    VignetteSettings vignetteSettings_{};
    SmoothingSettings smoothingSettings_{};
    GaussianSettings gaussianSettings_{};
    OutlineSettings outlineSettings_{};
    RadialBlurSettings radialBlurSettings_{};
    DissolveSettings dissolveSettings_{};
    RandomSettings randomSettings_{};


    D3D12_RESOURCE_STATES renderTextureState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;

    ComPtr<ID3D12RootSignature> copyImageRootSignature_;
    ComPtr<ID3D12PipelineState> copyImagePipelineState_;

    bool enableGrayScale_ = true;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> normalCopyPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> grayScalePipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> postEffectPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> smoothingPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> gaussianPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> outlinePipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> radialBlurPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> dissolvePipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> randomPipelineState_;


    uint32_t width_ = 1280;

    uint32_t height_ = 720;
    
    float randomTime_ = 0.0f;
};