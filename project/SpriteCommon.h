#pragma once
#include "DirectXCommon.h"
#include"Mymath.h"
class SpriteCommon
{
public:
	void Initialize(DirectXCommon*dxCommon);
	DirectXCommon* GetDxCommon()const { return dxCommon_; }
	// View / Projection 取得
	const MyMath::Matrix4x4& GetViewMatrix() const {
		return viewMatrix_;
	}
	const MyMath::Matrix4x4& GetProjectionMatrix() const {
		return projectionMatrix_;
	}


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

private:
	MyMath::Matrix4x4 viewMatrix_;
	MyMath::Matrix4x4 projectionMatrix_;
};

