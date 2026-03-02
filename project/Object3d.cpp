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

	//objの参照しているテクスチャファイル読み込み
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	//読み込んだテクスチャの番号を取得
	TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);

	modelData = LoadObjFile("resources", "plane.obj");

	//Transformの初期値設定
	//スケール、回転、平行移動
	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	cameraTransform = { {1.0f,1.0f,1.0f},{0.3f,0.0f,0.0f},{0.0f,4.0f,-10.0f} };

	CreateTransformationMatrix();

	CreateDirectionalLight();
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

void Object3d::CreateTransformationMatrix() {
	transformationMatrixResource_ = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
	transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

	transformationMatrixData_->WVP = Math::MakeIdentity4x4();
	transformationMatrixData_->World = Math::MakeIdentity4x4();
}

void Object3d::CreateDirectionalLight() {
	directionalLightResource_ = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(DirectionalLight));

	directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));

	//初期値
	directionalLightData_->color = { 1.0f,1.0f,1.0f,1.0f };//白
	directionalLightData_->direction = { 0.0f,-1.0f,0.0f };//下向き
}

void Object3d::Update() {
	// =========================
  // 1. WorldMatrix作成
  // =========================
	Math::Matrix4x4 scaleMatrix =
		Math::MakeScaleMatrix(transform.scale);

	Math::Matrix4x4 rotateMatrix =
		Math::MakeRotateXYZMatrix(transform.rotate);

	Math::Matrix4x4 translateMatrix =
		Math::MakeTranslateMatrix(transform.translate);

	Math::Matrix4x4 worldMatrix =
		scaleMatrix * rotateMatrix * translateMatrix;

	// =========================
	// 2. ViewMatrix作成（カメラ）
	// =========================
	Math::Matrix4x4 cameraMatrix =
		Math::MakeAffineMatrix(
			cameraTransform.scale,
			cameraTransform.rotate,
			cameraTransform.translate
		);

	Math::Matrix4x4 viewMatrix =
		Math::Inverse(cameraMatrix);

	// =========================
	// 3. ProjectionMatrix作成
	// =========================
	Math::Matrix4x4 projectionMatrix =
		Math::MakePerspectiveFovMatrix(
			0.45f,                                   // FOV
			float(WinApp::kClientWidth) /
			float(WinApp::kClientHeight),             // アスペクト比
			0.1f,                                     // nearZ
			100.0f                                    // farZ
		);

	// =========================
	// 4. WVP計算
	// =========================
	Math::Matrix4x4 worldViewProjectionMatrix =
		worldMatrix * viewMatrix * projectionMatrix;

	// =========================
	// 5. 定数バッファに書き込み
	// =========================
	transformationMatrixData_->World = worldMatrix;
	transformationMatrixData_->WVP = worldViewProjectionMatrix;
}

void Object3d::Draw()
{

}
