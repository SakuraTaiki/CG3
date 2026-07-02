#include "TextureManager.h"
#include <vector>
#include <cassert>
#include <format>
#include "externals/DirectXTex/DirectXTex.h"

using namespace Microsoft::WRL;

// 文字列変換用
static std::wstring ConvertString(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// データ転送関数
[[nodiscard]]
ComPtr<ID3D12Resource> UploadTextureData(ID3D12Device* device, const DirectX::ScratchImage& mipImages) {
    const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
    D3D12_RESOURCE_DESC textureDesc{};
    textureDesc.Width = UINT(metadata.width);
    textureDesc.Height = UINT(metadata.height);
    textureDesc.MipLevels = UINT16(metadata.mipLevels);
    textureDesc.DepthOrArraySize = UINT16(metadata.arraySize);
    textureDesc.Format = metadata.format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

    D3D12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_CUSTOM, D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0, 1, 1 };

    ComPtr<ID3D12Resource> resource;
    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
    assert(SUCCEEDED(hr));

    const DirectX::Image* intermediateImages = mipImages.GetImages();
    for (size_t i = 0; i < metadata.mipLevels; ++i) {
        const DirectX::Image& img = intermediateImages[i];
        void* pData = nullptr;
        hr = resource->Map(UINT(i), nullptr, &pData);
        if (SUCCEEDED(hr)) {
            const uint8_t* src = img.pixels;
            uint8_t* dst = static_cast<uint8_t*>(pData);
            for (size_t y = 0; y < img.height; ++y) {
                memcpy(dst, src, img.rowPitch);
                src += img.rowPitch;
                dst += img.rowPitch;
            }
            resource->Unmap(UINT(i), nullptr);
        }
    }
    return resource;
}

void TextureManager::Initialize(DirectXCommon* dxCommon,SrvManager*srvManager) {
    dxCommon_ = dxCommon;
    srvManager_ = srvManager;

    assert(dxCommon_);
    assert(srvManager_);
}

uint32_t TextureManager::LoadTexture(const std::string& filePath) {
    if (fileMap_.contains(filePath)) {
        return fileMap_[filePath];
    }

    assert(textures_.size() < kMaxTextures);

    DirectX::ScratchImage image;
    DirectX::ScratchImage mipImages;

    std::wstring wFilePath = ConvertString(filePath);
    HRESULT hr;

    if (filePath.ends_with(".dds")) {
        hr = DirectX::LoadFromDDSFile(
            wFilePath.c_str(),
            DirectX::DDS_FLAGS_NONE,
            nullptr,
            image);
    } else {
        hr = DirectX::LoadFromWICFile(
            wFilePath.c_str(),
            DirectX::WIC_FLAGS_FORCE_SRGB,
            nullptr,
            image);
    }

    if (FAILED(hr)) {
        std::string message = "Failed to load texture: " + filePath + "\n";
        OutputDebugStringA(message.c_str());
        assert(false);
        return 0;
    }

    const DirectX::TexMetadata& metadata = image.GetMetadata();

    if (DirectX::IsCompressed(metadata.format)) {
        mipImages = std::move(image);
    } else {
        hr = DirectX::GenerateMipMaps(
            image.GetImages(),
            image.GetImageCount(),
            image.GetMetadata(),
            DirectX::TEX_FILTER_SRGB,
            0,
            mipImages);

        assert(SUCCEEDED(hr));
    }

    const DirectX::TexMetadata& finalMetadata = mipImages.GetMetadata();

    D3D12_RESOURCE_DESC textureDesc{};
    textureDesc.Width = UINT(finalMetadata.width);
    textureDesc.Height = UINT(finalMetadata.height);
    textureDesc.MipLevels = UINT16(finalMetadata.mipLevels);
    textureDesc.DepthOrArraySize = UINT16(finalMetadata.arraySize);
    textureDesc.Format = finalMetadata.format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION(finalMetadata.dimension);

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_CUSTOM;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

    hr = dxCommon_->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&textureResource));

    assert(SUCCEEDED(hr));

    for (size_t arrayIndex = 0; arrayIndex < finalMetadata.arraySize; ++arrayIndex) {
        for (size_t mipIndex = 0; mipIndex < finalMetadata.mipLevels; ++mipIndex) {

            const DirectX::Image* img = mipImages.GetImage(
                mipIndex,
                arrayIndex,
                0);

            assert(img);

            UINT subresourceIndex =
                UINT(mipIndex + arrayIndex * finalMetadata.mipLevels);

            hr = textureResource->WriteToSubresource(
                subresourceIndex,
                nullptr,
                img->pixels,
                UINT(img->rowPitch),
                UINT(img->slicePitch));

            assert(SUCCEEDED(hr));
        }
    }

    TextureData data{};
    data.resource = textureResource;
    data.resourceDesc = textureResource->GetDesc();

    uint32_t index = static_cast<uint32_t>(textures_.size());

    assert(srvManager_->CheckCanAllocate());

    data.srvIndex = srvManager_->Allocate();
    data.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(data.srvIndex);
    data.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(data.srvIndex);

    if (finalMetadata.IsCubemap()) {
        srvManager_->CreateSRVForTextureCube(
            data.srvIndex,
            data.resource.Get(),
            data.resourceDesc.Format,
            UINT(finalMetadata.mipLevels)
        );
    } else {
        srvManager_->CreateSRVForTexture2D(
            data.srvIndex,
            data.resource.Get(),
            data.resourceDesc.Format,
            UINT(finalMetadata.mipLevels)
        );
    }

    textures_.push_back(data);

    fileMap_[filePath] = index;

    return index;
}
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureHandle) {
    return textures_[textureHandle].srvHandleGPU;
}

const D3D12_RESOURCE_DESC& TextureManager::GetResourceDesc(uint32_t textureHandle) {
    return textures_[textureHandle].resourceDesc;
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleCPU(uint32_t textureHandle)
{
    assert(textureHandle < textures_.size());

    return textures_[textureHandle]
        .srvHandleCPU;
}
