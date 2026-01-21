#pragma once
#include <string>
#include"DirectXCommon.h"
class TextureManager
{
private:
	static TextureManager* instance;
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;

public:

	void Initialize();

	

	void LoadTexture(const std::string& filePath);

	static TextureManager* GetInstance();
	void Finalize();
private:
	struct TextureData {
		std::string filePath;
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};
	std::vector<TextureData>textureDates;
};

