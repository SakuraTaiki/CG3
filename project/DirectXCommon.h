#pragma once
#include<Windows.h>
#include "WinApp.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "Logger.h"
#include <dxcapi.h>
#include<cstdint>
#include "externals/DirectXTex/DirectXTex.h"
#include<chrono>

class DirectXCommon {
public:
    void Initialize(WinApp* winApp);

	static const uint32_t kMaxSRVCount;

	ID3D12Device* GetDevice() const { return device_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

	DirectX::ScratchImage LoadTexture(const std::string& filePath);

	void UploadTextureData(
		Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
		const DirectX::ScratchImage& mipImages);

	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
		const std::wstring& filePath,
		const wchar_t* profile);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE heapType,
		UINT numDescriptors,
		bool shaderVisible);

	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
		const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
		uint32_t descriptorSize,
		uint32_t index);

	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
		const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
		uint32_t descriptorSize,
		uint32_t index);

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index) const;

	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index) const;

	ID3D12DescriptorHeap* GetSRVDescriptorHeap() const { return srvDescriptorHeap_.Get(); }
	void PreDraw();
	void PostDraw();
private:

	WinApp* winApp_ = nullptr;

	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;

	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;

	Microsoft::WRL::ComPtr<ID3D12Device> device_;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_; //  ComPtrへ変更
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_; //  ComPtrへ変更
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2]; //  ComPtrへ変更

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;

	// --- Fence (ComPtr へ変更) ---
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_; //  ComPtrへ変更
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_ = nullptr;

	// --- DXCompiler (ComPtr へ変更) ---
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_; //  ComPtrへ変更
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_; // ComPtrへ変更
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_; //  ComPtrへ変更

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2];

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;

	uint32_t rtvDescriptorSize_ = 0;
	uint32_t srvDescriptorSize_ = 0;
	uint32_t dsvDescriptorSize_ = 0;

	std::chrono::steady_clock::time_point reference_;

	D3D12_VIEWPORT viewport_;
	D3D12_RECT scissorRect_;

	void InitializeDevice();
	void InitializeCommand();     // コマンド関連 (キュー、アロケータ、リスト) の作成
	void InitializeDescriptorHeaps();
	void InitializeSwapchain();   // スワップチェーン、RTVヒープ、RTVの作成
	void InitializeRTV();         // RTVの設定と作成
	void InitializeDepthBufferResource();
	void InitializeDSV();
	void InitializeFence();       // フェンスの作成
	void InitializeViewport();
	void InitializeScissor();
	void InitializeDXC();         // DXCコンパイラの初期化
	void InitializeImGui();       // ImGuiのDirectX部分の初期化


	void InitializeFixFPS();
	// FPS固定更新
	void UpdateFixFPS();

};