#pragma once

#include "WinApp.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "Logger.h"
#include <dxcapi.h>
#include "externals/DirectXTex/DirectXTex.h"
#include<chrono>

class DirectXCommon {
public:
    void Initialize(WinApp* winApp);

    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

    void PreDraw();
    void PostDraw();

    //リソース生成関数
    Microsoft::WRL::ComPtr<ID3D12Resource>CreateBufferResource(size_t sizeInBytes);
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);
    Microsoft::WRL::ComPtr<ID3D12Resource>CreateTextureResource(const DirectX::TexMetadata& metadata);
    Microsoft::WRL::ComPtr<ID3D12Resource>UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages);
    static DirectX::ScratchImage LoadTexture(const std::string& filePath);

    ID3D12Device* GetDevice()const { return device.Get(); }
    ID3D12GraphicsCommandList* GetCommandList()const { return commandList.Get(); }

    static D3D12_CPU_DESCRIPTOR_HANDLE
        GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& heap,
            uint32_t descriptorSize, uint32_t index);

    static D3D12_GPU_DESCRIPTOR_HANDLE
        GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& heap,
            uint32_t descriptorSize, uint32_t index);

    Microsoft::WRL::ComPtr<ID3D12Resource>
        CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device,
            int32_t width, int32_t height);

    //========================
    // Descriptor Heap
    //========================
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

    Microsoft::WRL::ComPtr<IDxcBlob>CompileShander(const std::wstring& filePath, const wchar_t* profile);

private:
    //========================
    // DirectX 基本オブジェクト
    //========================
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> device;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
    Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2];

    
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

    UINT descriptorSizeSRV = 0;

    //========================
    // DepthStencil 用
    //========================
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource;

    //========================
    // Fence
    //========================
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    uint64_t fenceValue = 0;
    HANDLE fenceEvent = nullptr;

    //========================
    // Viewport / Scissor
    //========================
    D3D12_VIEWPORT viewport{};
    D3D12_RECT scissorRect{};

    WinApp* winApp = nullptr;

  
    //========================
    //SghaderCompile
    //========================
    

    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxCompiler;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;

    // Logger 用
    std::ostream logFile;

   

private:
    //========================
    // Utility functions
    //========================
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
        CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

    

   
private:
   //FPS固定化関数
    void InitializeFixFPS();
    //FPS固定更新
    void UpdateFixFPS();

    //記録時間(FPS固定用)
    std::chrono::steady_clock::time_point reference_;

};