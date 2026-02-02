#pragma once
#include <string>
#include<vector>
#include<wrl.h>
#include<d3d12.h>
#include "externals/DirectXTex/DirectXTex.h"

class DirectXCommon;

class TextureManager
{
public:

	static TextureManager* GetInstance();
	void SetDirectXCommon(DirectXCommon* dxCommon);

	void Initialize();
	void Finalize();

	void LoadTexture(const std::string& filePath);

	uint32_t GetTextureIndexByFilePath(const std::string& filePath);

	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);

private:
	static TextureManager* instance;

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;

	static uint32_t kSRVIndexTop;
private:

	struct TextureData {
		std::string filePath;
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};

	std::vector<TextureData>textureDates;
	DirectXCommon* dxCommon_ = nullptr;
};