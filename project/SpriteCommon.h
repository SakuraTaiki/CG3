#pragma once
#include "DirectXCommon.h"
class SpriteCommon
{
public:
	void Initialize(DirectXCommon*dxCommon);
	DirectXCommon* GetDxCommon()const { return dxCommon_; }
	void Draw();
private:
	void CreateRootSignature();
	void CreateGraphicsPipeline();
	DirectXCommon* dxCommon_=nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
	//シアライズしてばいなりにする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	void SetGraphicsRootSignature();
	void SetPipeLineState();
	void IASetPrimitiveTopology();
};

