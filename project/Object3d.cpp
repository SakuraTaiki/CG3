#include "Object3d.h"
#include"Object3dCommon.h"
#include <cassert>
#include "Math.h"
#include "WinApp.h"
#include <algorithm>
#include"TextureManager.h"


void Object3d::Initialize(Object3dCommon* object3dCommon)
{
	this->object3dCommon = object3dCommon;
	modelData = LoadObjFile("resources", "plane.obj");

	CreateVertexData();
}

void Object3d::CreateVertexData()
{
	ID3D12Device* device = object3dCommon->GetDxCommon()->GetDevice();
	HRESULT hr = S_FALSE;

	//1--VertexResourceを作る
	const size_t kVertexCount = 4;
	// VertexResource (4頂点)
	vertexResource_ = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * kVertexCount);

	//vertexResourceにデータを書き込むためのアドレスを取得
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

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

	// VertexBufferView
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * kVertexCount;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);
}

void Object3d::CreateMaterial() {
	materialResource_ = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(Material));
	assert(materialResource_);

	HRESULT hr = materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	assert(SUCCEEDED(hr));

	materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData_->enableLighting = false; // false = 0

	materialData_->uvTransform = Math::MakeIdentity4x4();
}