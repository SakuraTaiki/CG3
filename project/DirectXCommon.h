#pragma once

#include "WinApp.h"

#include <d3d12.h>
#include<dxgi1_6.h>
#include<wrl.h>


class DirectXCommon {
public:
	void Initialize(WinApp*winApp);
	

private:
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);
	WinApp* winApp = nullptr;

};

