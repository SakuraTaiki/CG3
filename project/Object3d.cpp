#include "Object3d.h"
#include"Object3dCommon.h"
#include <cassert>
#include "Math.h"
#include "WinApp.h"
#include <algorithm>
#include"TextureManager.h"
#include"Model.h"
#include"ModelManager.h"

#include<fstream>
#include<sstream>

void Object3d::Initialize(Object3dCommon* object3dCommon)
{

	object3dCommon_ = object3dCommon;

	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	cameraTransform = { {1.0f,1.0f,1.0f},{0.3f,0.0f,0.0f},{0.0f,4.0f,-10.0f} };

	// 行列・ライトだけ作る
	CreateTransformationMatrix();
	CreateDirectionalLight();
}

void Object3d::SetModel(const std::string& filePath)
{
	model_ = ModelManager::GetInstance()->FindModel(filePath);
}

void Object3d::CreateTransformationMatrix() {
	transformationMatrixResource_ = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
	transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

	transformationMatrixData_->WVP = Math::MakeIdentity4x4();
	transformationMatrixData_->World = Math::MakeIdentity4x4();
}

void Object3d::CreateDirectionalLight() {
	directionalLightResource_ = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(DirectionalLight));

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

	transform.rotate.y += 0.02f;
}

void Object3d::Draw()
{
	ID3D12GraphicsCommandList* commandList =
		object3dCommon_->GetDxCommon()->GetCommandList();

	object3dCommon_->SetCommonDrawSettings(commandList);

	// WVP
	commandList->SetGraphicsRootConstantBufferView(
		0,
		transformationMatrixResource_->GetGPUVirtualAddress()
	);

	// ライト
	commandList->SetGraphicsRootConstantBufferView(
		3,
		directionalLightResource_->GetGPUVirtualAddress()
	);

	if (model_) {
		model_->Draw();
	}
}