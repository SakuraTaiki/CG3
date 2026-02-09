#pragma once
#include "DirectXCommon.h"
#include <d3d12.h>
#include <wrl.h>
class Object3dCommon
{
public:
	void Initialize(DirectXCommon* dxCommon);

	ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }

	ID3D12PipelineState* GetPipelineState() const { return graphicsPipelineState_.Get(); }

private:
	void CreateRootSignature();
	void CreateGraphicsPipelineState();
	DirectXCommon* dxCommon_;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
};

