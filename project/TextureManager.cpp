#include "TextureManager.h"
#include"DirectXCommon.h"
#include "StringUtility.h"
#include <algorithm>


TextureManager* TextureManager::instance = nullptr;
uint32_t TextureManager::kSRVIndexTop = 1;

TextureManager* TextureManager::GetInstance()
{
    if (instance == nullptr) {
        instance = new TextureManager;
    }
    return instance;
}

void TextureManager::SetDirectXCommon(DirectXCommon* dxCommon) {
    dxCommon_ = dxCommon;
}

void TextureManager::Initialize() {
    textureDates.reserve(DirectXCommon::kMaxSRVCount);
}

void TextureManager::Finalize() {
    delete instance;
    instance = nullptr;
}


void TextureManager::LoadTexture(const std::string& filePath) {

    DirectXCommon* dxCommon = dxCommon_;

    auto it = std::find_if(textureDates.begin(), textureDates.end(), [&](const TextureData&data) {return data.filePath == filePath; });

    if (it != textureDates.end()) {
        return;
    }

    assert(textureDates.size() + kSRVIndexTop < DirectXCommon::kMaxSRVCount);

    DirectX::ScratchImage image{};
    std::wstring filePathW = StringUtility::ConvertString(filePath);
    HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
    assert(SUCCEEDED(hr));

    //ミップマップの作成
    DirectX::ScratchImage mipImages{};
    hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
    assert(SUCCEEDED(hr));

    textureDates.resize(textureDates.size() + 1);

    TextureData& textureData = textureDates.back();

    textureData.filePath = filePath;

    textureData.metadata = mipImages.GetMetadata();

    textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);

    uint32_t srvIndex = static_cast<uint32_t>(textureDates.size() - 1) + kSRVIndexTop;

    textureData.srvHandleCPU = dxCommon_->GetSRVCPUDescriptorHandle(srvIndex);

    textureData.srvHandleGPU = dxCommon_->GetSRVGPUDescriptorHandle(srvIndex);


    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

    srvDesc.Format = textureData.metadata.format;

    srvDesc.Shader4ComponentMapping= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels =UINT(textureData.metadata.mipLevels);

    dxCommon_->GetDevice()->CreateShaderResourceView(textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);

    dxCommon->GetDevice()->CreateShaderResourceView(
        textureData.resource.Get(),
        &srvDesc,
        textureData.srvHandleCPU
    );

    dxCommon->UploadTextureData(textureData.resource, mipImages);
}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath)
{
    auto it = std::find_if(textureDates.begin(), textureDates.end(), [&](TextureData& textureData) {return textureData.filePath == filePath; });
    if (it != textureDates.end()) {
        uint32_t textureIndex = static_cast<uint32_t>(std::distance(textureDates.begin(), it));
        return textureIndex;
    }

    if (it != textureDates.end()) {
        // インデックス = it - begin
        uint32_t textureIndex =
            static_cast<uint32_t>(std::distance(textureDates.begin(), it));

        return textureIndex;
    }

    assert(0);
    return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureIndex)
{
    assert(textureIndex < textureDates.size());
    TextureData& textureData = textureDates[textureIndex];
    return textureData.srvHandleGPU;
}

// メタデータを取得
const DirectX::TexMetadata& TextureManager::GetMetaData(uint32_t textureIndex)
{
    // 範囲外指定違反チェック
    assert(textureIndex < textureDates.size());

    // テクスチャデータの参照を取得
    TextureData& textureData = textureDates[textureIndex];

    // メタデータを返す
    return textureData.metadata;
}

