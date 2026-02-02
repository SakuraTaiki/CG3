#include "Sprite.h"
#include "SpriteCommon.h"
#include <cassert>
#include "Math.h"
#include "WinApp.h"
#include <algorithm>
#include"TextureManager.h"
void Sprite::Initialize(SpriteCommon* spriteCommon,std::string textureFilePath) {

	this->spriteCommon_ = spriteCommon;
	textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
	assert(this->spriteCommon_ != nullptr);

	CreateVertexData();

	CreateMaterial();

	CreateTransformationMatrix();
}

void Sprite::CreateVertexData()
{
	ID3D12Device* device = spriteCommon_->GetDxCommon()->GetDevice();
	HRESULT hr = S_FALSE;

	// --- 1. VertexResource / IndexResource を作る ---
	const size_t kVertexCount = 4; // 矩形は通常4頂点
	const size_t kIndexCount = 6;  // 矩形は通常6インデックス

	// VertexResource (4頂点)
	vertexResource_ = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * kVertexCount);
	// IndexResource (6インデックス)
	indexResource_ = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * kIndexCount);

	// --- 2. VertexResource / IndexResource にデータを書き込むためのアドレスを取得 ---
	// VertexData* vertexData_ = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	// uint32_t* indexData_ = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

	// --- 3. VertexData の初期値設定 (main.cpp から移植) ---
	// ※ ここではスプライトのサイズを仮で 128x128 と設定するロジックを移植します
	// main.cppの座標 {0.0f,360.0f}, {0.0f,0.0f}, {640.0f,360.0f}, {640.0f,0.0f}
	float left = 0.0f;
	float top = 0.0f;
	float right = 1.0f;
	float bottom = 1.0f;

	vertexData_[0].position = { left, bottom, 0.0f, 1.0f };    // 左下
	vertexData_[0].texcoord = { 0.0f, 1.0f };
	vertexData_[1].position = { left, top, 0.0f, 1.0f };       // 左上
	vertexData_[1].texcoord = { 0.0f, 0.0f };
	vertexData_[2].position = { right, bottom, 0.0f, 1.0f };   // 右下
	vertexData_[2].texcoord = { 1.0f, 1.0f };
	vertexData_[3].position = { right, top, 0.0f, 1.0f };      // 右上
	vertexData_[3].texcoord = { 1.0f, 0.0f };
	// Normalは一旦0で埋める
	vertexData_[0].normal = { 0.0f, 0.0f, -1.0f };
	vertexData_[1].normal = { 0.0f, 0.0f, -1.0f };
	vertexData_[2].normal = { 0.0f, 0.0f, -1.0f };
	vertexData_[3].normal = { 0.0f, 0.0f, -1.0f };

	// IndexData の設定 (main.cpp から移植)
	indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2; // 1枚目の三角形 (左下、左上、右下)
	indexData_[3] = 1; indexData_[4] = 3; indexData_[5] = 2; // 2枚目の三角形 (左上、右上、右下)

	// --- 4. VertexBufferView / IndexBufferView を作成する ---
	// VertexBufferView
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * kVertexCount;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// IndexBufferView
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * kIndexCount;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
}

void Sprite::CreateMaterial()
{

	materialResource_ = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));
	assert(materialResource_);

	HRESULT hr = materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	assert(SUCCEEDED(hr));

	materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData_->enableLighting = false; // false = 0

	materialData_->uvTransform = Math::MakeIdentity4x4();
}

void Sprite::CreateTransformationMatrix()
{
	transformationMatrixResource_ = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));

	transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

	transformationMatrixData_->WVP = Math::MakeIdentity4x4();
	transformationMatrixData_->World = Math::MakeIdentity4x4();
}

void Sprite::UpdateTransformationMatrix() {
	Math::Matrix4x4 worldMatrix = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	Math::Matrix4x4 viewMatrix = Math::MakeIdentity4x4();

	Math::Matrix4x4 projectionMatrix = Math::MakeOrthorgraphicMatrix(
		0.0f,
		0.0f,                          // [FIX] ここを 0.0f に変更 (Top = 0)
		(float)WinApp::kClientWidth,
		(float)WinApp::kClientHeight,  // [FIX] ここを Height に変更 (Bottom = Height)
		0.0f,
		100.0f
	);

	Math::Matrix4x4 worldViewProjectionMatrix = Math::Multiply(worldMatrix, Math::Multiply(viewMatrix, projectionMatrix));

	transformationMatrixData_->WVP = worldViewProjectionMatrix;

	transformationMatrixData_->World = worldMatrix;
}



void Sprite::Update()
{
	float left = 0.0f - anchorPoint_.x;
	float right = 1.0f - anchorPoint_.x;
	float top = 0.0f - anchorPoint_.y;
	float bottom = 1.0f - anchorPoint_.y;

	vertexData_[0].position = { left, bottom, 0.0f, 1.0f };

	vertexData_[1].position = { left, top, 0.0f, 1.0f };

	vertexData_[2].position = { right, bottom, 0.0f, 1.0f };

	vertexData_[3].position = { right, top, 0.0f, 1.0f };

	transform_.translate = { position_.x,position_.y,0.0f };

	transform_.scale = { size_.x, size_.y, 1.0f };

	UpdateTransformationMatrix();
}
void Sprite::Draw(ID3D12GraphicsCommandList* commandList) {
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->IASetIndexBuffer(&indexBufferView_);
	commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex));
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

