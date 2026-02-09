#include "Object3dCommon.h"
#include"DirectXCommon.h"
#include"WinApp.h"
#include"dxcapi.h"
#include<cassert>

using namespace Microsoft::WRL;

void Object3dCommon::SetCommonDrawSettings(ID3D12GraphicsCommandList* commandList) {

	// 1. ルートシグネチャをセットするコマンド
	commandList->SetGraphicsRootSignature(rootSignature_.Get());

	// 2. グラフィックスパイプラインステートをセットするコマンド
	commandList->SetPipelineState(graphicsPipelineState_.Get());

	// 3. プリミティブトポロジーをセットするコマンド
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}


void Object3dCommon::Initialize(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	assert(dxCommon_ != nullptr);

	// グラフィックスパイプラインの生成を呼び出す
	CreateGraphicsPipelineState();
}
