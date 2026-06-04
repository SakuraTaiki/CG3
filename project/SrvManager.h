#pragma once
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

class DirectXCommon;

class SrvManager {
public:
    static const uint32_t kMaxSRVCount;

    void Initialize(DirectXCommon* dxCommon);

    uint32_t Allocate();
    bool CheckCanAllocate() const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

    void CreateSRVForTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT mipLevels);
    void CreateSRVForTextureCube(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT mipLevels);

    void PreDraw();
    void SetGraphicsRootDescriptorTable(UINT rootParameterIndex, uint32_t srvIndex);

    static SrvManager* GetInstance();

    ID3D12DescriptorHeap* GetDescriptorHeap() const {
        return descriptorHeap_.Get();
    }

private:
    DirectXCommon* directXCommon_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
    uint32_t descriptorSize_ = 0;
    uint32_t useIndex_ = 0;
};