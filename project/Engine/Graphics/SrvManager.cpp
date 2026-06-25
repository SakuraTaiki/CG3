#include "SrvManager.h"
#include "DirectXCommon.h"
#include <cassert>

SrvManager* SrvManager::GetInstance() {
    static SrvManager instance;
    return &instance;
}

const uint32_t SrvManager::kMaxSRVCount = 512;

void SrvManager::Initialize(DirectXCommon* dxCommon) {
    assert(dxCommon);
    directXCommon_ = dxCommon;

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = kMaxSRVCount;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = directXCommon_->GetDevice()->CreateDescriptorHeap(
        &desc,
        IID_PPV_ARGS(&descriptorHeap_)
    );
    assert(SUCCEEDED(hr));

    descriptorSize_ =
        directXCommon_->GetDevice()->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );
}

uint32_t SrvManager::Allocate() {
    assert(CheckCanAllocate());

    uint32_t index = useIndex_;
    useIndex_++;

    return index;
}

bool SrvManager::CheckCanAllocate() const {
    return useIndex_ < kMaxSRVCount;
}

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUDescriptorHandle(uint32_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        descriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    handle.ptr += descriptorSize_ * index;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUDescriptorHandle(uint32_t index) {
    D3D12_GPU_DESCRIPTOR_HANDLE handle =
        descriptorHeap_->GetGPUDescriptorHandleForHeapStart();

    handle.ptr += descriptorSize_ * index;
    return handle;
}

void SrvManager::CreateSRVForTexture2D(
    uint32_t srvIndex,
    ID3D12Resource* pResource,
    DXGI_FORMAT format,
    UINT mipLevels
) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = mipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    directXCommon_->GetDevice()->CreateShaderResourceView(
        pResource,
        &srvDesc,
        GetCPUDescriptorHandle(srvIndex)
    );
}

void SrvManager::CreateSRVForTextureCube(
    uint32_t srvIndex,
    ID3D12Resource* pResource,
    DXGI_FORMAT format,
    UINT mipLevels
) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MipLevels = mipLevels;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

    directXCommon_->GetDevice()->CreateShaderResourceView(
        pResource,
        &srvDesc,
        GetCPUDescriptorHandle(srvIndex)
    );
}

void SrvManager::CreateSRVForStructuredBuffer(
    uint32_t srvIndex,
    ID3D12Resource* pResource,
    UINT numElements,
    UINT structureByteStride
) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.NumElements = numElements;
    srvDesc.Buffer.StructureByteStride = structureByteStride;

    directXCommon_->GetDevice()->CreateShaderResourceView(
        pResource,
        &srvDesc,
        GetCPUDescriptorHandle(srvIndex)
    );
}

void SrvManager::CreateUAVForStructuredBuffer(
    uint32_t srvIndex,
    ID3D12Resource* pResource,
    UINT numElements,
    UINT structureByteStride
) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = numElements;
    uavDesc.Buffer.StructureByteStride = structureByteStride;
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    directXCommon_->GetDevice()->CreateUnorderedAccessView(
        pResource,
        nullptr,
        &uavDesc,
        GetCPUDescriptorHandle(srvIndex)
    );
}

void SrvManager::PreDraw() {
    ID3D12DescriptorHeap* heaps[] = {
        descriptorHeap_.Get()
    };

    directXCommon_->GetCommandList()->SetDescriptorHeaps(1, heaps);
}

void SrvManager::SetGraphicsRootDescriptorTable(UINT rootParameterIndex, uint32_t srvIndex) {
    directXCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(
        rootParameterIndex,
        GetGPUDescriptorHandle(srvIndex)
    );
}
