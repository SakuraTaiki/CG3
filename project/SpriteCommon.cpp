#include "SpriteCommon.h"
#include"Logger.h"
#include "Input.h"

void SpriteCommon::Initialize(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	CreateRootSignature();
	CreateGraphicsPipeline();

	viewMatrix_ = MyMath::Matrix4x4::MakeIdentity();

	// 画面サイズに合わせた正射影
	projectionMatrix_ =
		MyMath::Matrix4x4::OrthographicLH(
			1280.0f,   // 画面幅
			720.0f,    // 画面高さ
			0.0f,
			100.0f
		);
}


void SpriteCommon::CreateRootSignature()
{
	

	//rootsignantrue作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignatrue{};

	descriptionRootSignatrue.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;//0から始まる
	descriptorRange[0].NumDescriptors = 1;//数は一つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//srvを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;//自動計算

	//Rootparameter作成　複数設定できるので配列 今回は結果が一つだけなので長さが1の配列

	D3D12_ROOT_PARAMETER rootParmeters[4] = {};
	rootParmeters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//CBVを使う
	rootParmeters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//pixeShaderで使う
	rootParmeters[0].Descriptor.ShaderRegister = 0;//レジスタ番号と0バインド

	rootParmeters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//CVBを使う
	rootParmeters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;//vertexShaerで使う
	rootParmeters[1].Descriptor.ShaderRegister = 0;//レジスタ番号0を使う

	rootParmeters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//descriptorTableを使う
	rootParmeters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParmeters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParmeters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);//tableで利用する数

	//05_00で追加

	rootParmeters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//CBVを使う
	rootParmeters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderを使う
	rootParmeters[3].Descriptor.ShaderRegister = 1;//レジスタ番号1を使う


	descriptionRootSignatrue.pParameters = rootParmeters;//ルートパラメーターへのポインタ
	descriptionRootSignatrue.NumParameters = _countof(rootParmeters);//配列の長さ

	//samplerの設定お行う
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;//バイリニアフィルター
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//0 ~1の範囲外をリピート 
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//比較
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignatrue.pStaticSamplers = staticSamplers;
	descriptionRootSignatrue.NumStaticSamplers = _countof(staticSamplers);


	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignatrue, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr))
	{
		Logger::Log(Logger::logStream, reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	hr = dxCommon_->GetDevice()->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}

void SpriteCommon::CreateGraphicsPipeline()
{
	//inputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;


	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	//=======CG3_00_01 Blendの設定の追加============//

	D3D12_BLEND_DESC blendDescs{};
	blendDescs.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDescs.RenderTarget[0].BlendEnable = TRUE;
	blendDescs.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDescs.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDescs.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

	//========ALPHA値のBlendの設定(基本的に使わない)
	blendDescs.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDescs.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDescs.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;




	//rasiterzerstateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	//裏面時計回りに表示しない

	//Noneにすると両面描画
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	//三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;


	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//Depthの機能を無効化する
	depthStencilDesc.DepthEnable = false;
	
	//書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//比較関係はLessEqual　つまり透ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	//shaderをコンパイルする
	ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompileShander(L"resources/shaders/Object3d.VS.hlsl",
		L"vs_6_0");
	assert(vertexShaderBlob != nullptr);
	ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShander(L"resources/shaders/Object3d.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);



	//psoを作成する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
	vertexShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
	pixelShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.BlendState = blendDescs;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;


	//DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//巻き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//利用するトロポジの情報のタイプ
	graphicsPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//どのように画面に色を打ち込むのか設定
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(
		&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&pipelineState_)
	);
	assert(SUCCEEDED(hr));
	
}


void SpriteCommon::Draw()
{
	SetGraphicsRootSignature();
	SetPipeLineState();
	IASetPrimitiveTopology();
}




void SpriteCommon::SetGraphicsRootSignature()
{
	auto commandList = dxCommon_->GetCommandList();
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
}

void SpriteCommon::SetPipeLineState()
{
	auto commandList = dxCommon_->GetCommandList();

	commandList->SetPipelineState(pipelineState_.Get());
}

void SpriteCommon::IASetPrimitiveTopology()
{
	auto commandList = dxCommon_->GetCommandList();

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

}
