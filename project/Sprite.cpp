#include "Sprite.h"
#include "SpriteCommon.h"
void Sprite::Initialize(SpriteCommon* spriteCommon) {

	this->spriteCommon_ = spriteCommon;

	CreateVertexResource();
	CreateIndexResource();
	CreateVertexBufferView();
	CreateIndexBufferView();
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

