#pragma once
#include "DirectXCommon.h"
#include <d3d12.h>
#include <wrl.h>
class SpriteCommon
{
public:
	void Initialize(DirectXCommon*dxCommon);
	
	void PreDraw(ID3D12GraphicsCommandList* commandList);

	void SetCommonDrawSettings(ID3D12GraphicsCommandList* commandList);

	ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }

	ID3D12PipelineState* GetPipelineState() const { return graphicsPipelineState_.Get(); }

	DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:
	void CreateRootSignature();
	void CreateGraphicsPipelineState();
	DirectXCommon* dxCommon_=nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
	
};

