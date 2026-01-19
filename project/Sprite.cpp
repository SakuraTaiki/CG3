#include "Sprite.h"
#include "SpriteCommon.h"
void Sprite::Initialize(SpriteCommon* spriteCommon) {

	this->spriteCommon_ = spriteCommon;

	CreateVertexResource();
	CreateIndexResource();
	CreateVertexBufferView();
	CreateIndexBufferView();
	CreateMaterialResource();
	CreateTransformationMatrixResource();
}

void Sprite::CreateVertexResource()
{
	auto dxCommon = spriteCommon_->GetDxCommon();
	vertexResource_ = dxCommon->CreateBufferResource(sizeof(VertexData) * 4);
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

	// 左上
	vertexData_[0].position = { 0.0f, 360.0f, 0.0f, 1.0f };
	vertexData_[0].texcoord = { 0.0f, 1.0f };

	// 左下
	vertexData_[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	vertexData_[1].texcoord = { 0.0f, 0.0f };

	// 右上
	vertexData_[2].position = { 640.0f, 0.0f, 0.0f, 1.0f };
	vertexData_[2].texcoord = { 1.0f, 0.0f };

	// 右下
	vertexData_[3].position = { 640.0f, 360.0f, 0.0f, 1.0f };
	vertexData_[3].texcoord = { 1.0f, 1.0f };
}

void Sprite::CreateIndexResource()
{
	auto dxCommon = spriteCommon_->GetDxCommon();

	indexResource_ = dxCommon->CreateBufferResource(
		sizeof(uint32_t) * 6
	);

	indexResource_->Map(
		0,
		nullptr,
		reinterpret_cast<void**>(&indexData_)
	);

	indexData_[0] = 0;
	indexData_[1] = 1;
	indexData_[2] = 2;
	indexData_[3] = 1;
	indexData_[4] = 3;
	indexData_[5] = 2;
}

void Sprite::CreateVertexBufferView()
{
	vertexBufferView_.BufferLocation =
		vertexResource_->GetGPUVirtualAddress();

	vertexBufferView_.SizeInBytes =
		sizeof(VertexData) * 4;

	vertexBufferView_.StrideInBytes =
		sizeof(VertexData);
}

void Sprite::CreateIndexBufferView()
{
	indexBufferView_.BufferLocation =
		indexResource_->GetGPUVirtualAddress();

	indexBufferView_.SizeInBytes =
		sizeof(uint32_t) * 6;

	indexBufferView_.Format =
		DXGI_FORMAT_R32_UINT;
}

void Sprite::CreateMaterialResource()
{
	auto dxCommon = spriteCommon_->GetDxCommon();

	//マテリアル用定数バッファ
	materialResource_ =
		dxCommon->CreateBufferResource(sizeof(Material));

	materialResource_->Map(
		0,
		nullptr,
		reinterpret_cast<void**>(&materialData_)
	);

	// ===== マテリアル初期値（スライド通り）
	materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData_->enableLighting = false;
	materialData_->uvTransform = MyMath::Matrix4x4::MakeIdentity();

}

void Sprite::CreateTransformationMatrixResource()
{
	auto dxCommon = spriteCommon_->GetDxCommon();

	// 座標変換行列用定数バッファを作成
	transformationMatrixResource_ =
		dxCommon->CreateBufferResource(sizeof(TransformationMatrix));

	// MapしてCPU側ポインタを取得
	transformationMatrixResource_->Map(
		0,
		nullptr,
		reinterpret_cast<void**>(&transformationMatrixData_)
	);

	// ===== 初期化（スクショ通り）
	transformationMatrixData_->WVP =
		MyMath::Matrix4x4::MakeIdentity();

	transformationMatrixData_->World =
		MyMath::Matrix4x4::MakeIdentity();
}

void Sprite::SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srvHandle)
{
	textureSrvHandleGPU_ = srvHandle;
}

void Sprite::Update()
{
	// =========================
	// Transformを作成
	// =========================
	MyMath::Transform transform{};
	transform.scale = { 1.0f, 1.0f, 1.0f };
	transform.rotate = { 0.0f, 0.0f,rotation };
	transform.translate = {position.x,position.y, 0.0f };

	// =========================
	// WorldMatrix 作成
	// =========================
	MyMath::Matrix4x4 worldMatrix = transform.ToMatrix();

	// =========================
	// View / Projection を取得
	// =========================
	const MyMath::Matrix4x4& viewMatrix =
		spriteCommon_->GetViewMatrix();

	const MyMath::Matrix4x4& projectionMatrix =
		spriteCommon_->GetProjectionMatrix();

	// =========================
	// WVP 計算（スクショ通り）
	// =========================
	transformationMatrixData_->WVP =
		MyMath::Matrix4x4::Multiply(
			worldMatrix,
			MyMath::Matrix4x4::Multiply(viewMatrix, projectionMatrix)
		);

	transformationMatrixData_->World = worldMatrix;
}
void Sprite::Draw() {

	spriteCommon_->Draw();

	// コマンドリスト取得
	auto commandList = spriteCommon_->GetDxCommon()->GetCommandList();

	// =========================
	// VertexBufferView を設定
	// =========================
	commandList->IASetVertexBuffers(
		0,
		1,
		&vertexBufferView_
	);

	// =========================
	// IndexBufferView を設定
	// =========================
	commandList->IASetIndexBuffer(
		&indexBufferView_
	);

	// =========================
	// マテリアルCBの場所を設定
	// RootParameter[0]
	// =========================
	commandList->SetGraphicsRootConstantBufferView(
		0,
		materialResource_->GetGPUVirtualAddress()
	);

	// =========================
	// 座標変換行列CBの場所を設定
	// RootParameter[1]
	// =========================
	commandList->SetGraphicsRootConstantBufferView(
		1,
		transformationMatrixResource_->GetGPUVirtualAddress()
	);

	// =========================
	// SRV DescriptorTable の先頭を設定
	// RootParameter[2]
	// =========================
	commandList->SetGraphicsRootDescriptorTable(
		2,
		textureSrvHandleGPU_
	);

	// =========================
	// 描画（DrawCall）
	// =========================
	commandList->DrawIndexedInstanced(
		6, // Index数
		1, // Instance数
		0,
		0,
		0
	);
}

