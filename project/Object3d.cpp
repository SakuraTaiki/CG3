#include "Object3d.h"
#include"Object3dCommon.h"
#include <cassert>
#include "Math.h"
#include "WinApp.h"
#include <algorithm>
#include"TextureManager.h"

#include<fstream>
#include<sstream>

// ================================
// mtl読み込み
// ================================
Object3d::MaterialData Object3d::LoadMaterialTemplateFile(
	const std::string& directoryPath,
	const std::string& filename)
{
	MaterialData materialData;

	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	std::string line;

	while (std::getline(file, line)) {
		std::istringstream s(line);
		std::string identifier;
		s >> identifier;

		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}

	return materialData;
}

// ================================
// obj読み込み
// ================================
Object3d::ModelData Object3d::LoadObjFile(
	const std::string& directoryPath,
	const std::string& filename)
{
	ModelData modelData;

	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	std::vector<Math::Vector4> positions;
	std::vector<Math::Vector2> texcoords;
	std::vector<Math::Vector3> normals;

	std::string line;

	while (std::getline(file, line)) {
		std::istringstream s(line);
		std::string identifier;
		s >> identifier;

		// 頂点座標
		if (identifier == "v") {
			Math::Vector4 pos;
			s >> pos.x >> pos.y >> pos.z;
			pos.w = 1.0f;
			positions.push_back(pos);
		}

		// テクスチャ座標
		else if (identifier == "vt") {
			Math::Vector2 tex;
			s >> tex.x >> tex.y;
			tex.y = 1.0f - tex.y; // ←重要（上下反転）
			texcoords.push_back(tex);
		}

		// 法線
		else if (identifier == "vn") {
			Math::Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}

		// 面
		else if (identifier == "f") {
			std::string vertexStr;

			// 三角形前提（3つ）
			for (int i = 0; i < 3; i++) {
				s >> vertexStr;

				std::istringstream v(vertexStr);
				std::string indexStr;

				int indices[3] = { 0,0,0 };
				int idx = 0;

				while (std::getline(v, indexStr, '/')) {
					indices[idx++] = std::stoi(indexStr);
				}

				VertexData vertex{};

				vertex.position = positions[indices[0] - 1];
				if (!texcoords.empty()) {
					vertex.texcoord = texcoords[indices[1] - 1];
				}
				if (!normals.empty()) {
					vertex.normal = normals[indices[2] - 1];
				}

				modelData.vertices.push_back(vertex);
			}
		}

		// マテリアル読み込み
		else if (identifier == "mtllib") {
			std::string mtlFile;
			s >> mtlFile;
			modelData.material = LoadMaterialTemplateFile(directoryPath, mtlFile);
		}
	}

	return modelData;
}

void Object3d::Initialize(Object3dCommon* object3dCommon)
{
	this->object3dCommon = object3dCommon;

	modelData = LoadObjFile("resources/plane", "plane.obj");

	//objの参照しているテクスチャファイル読み込み
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	//読み込んだテクスチャの番号を取得
	TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);

	

	//Transformの初期値設定
	//スケール、回転、平行移動
	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	cameraTransform = { {1.0f,1.0f,1.0f},{0.3f,0.0f,0.0f},{0.0f,4.0f,-10.0f} };

	CreateVertexData();
	CreateMaterial();

	CreateTransformationMatrix();

	CreateDirectionalLight();
}

void Object3d::CreateVertexData()
{
	ID3D12Device* device = object3dCommon->GetDxCommon()->GetDevice();
	HRESULT hr = S_FALSE;

	//1--VertexResourceを作る
	const size_t kVertexCount = modelData.vertices.size();
	// VertexResource (4頂点)
	vertexResource_ = object3dCommon->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * kVertexCount);

	//vertexResourceにデータを書き込むためのアドレスを取得
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

	std::copy(modelData.vertices.begin(), modelData.vertices.end(), vertexData_);

	//// --- 3. VertexData の初期値設定 (main.cpp から移植) ---
	//// ※ ここではスプライトのサイズを仮で 128x128 と設定するロジックを移植します
	//// main.cppの座標 {0.0f,360.0f}, {0.0f,0.0f}, {640.0f,360.0f}, {640.0f,0.0f}
	//float left = 0.0f;
	//float top = 0.0f;
	//float right = 1.0f;
	//float bottom = 1.0f;

	//vertexData_[0].position = { left, bottom, 0.0f, 1.0f };    // 左下
	//vertexData_[0].texcoord = { 0.0f, 1.0f };
	//vertexData_[1].position = { left, top, 0.0f, 1.0f };       // 左上
	//vertexData_[1].texcoord = { 0.0f, 0.0f };
	//vertexData_[2].position = { right, bottom, 0.0f, 1.0f };   // 右下
	//vertexData_[2].texcoord = { 1.0f, 1.0f };
	//vertexData_[3].position = { right, top, 0.0f, 1.0f };      // 右上
	//vertexData_[3].texcoord = { 1.0f, 0.0f };
	//// Normalは一旦0で埋める
	//vertexData_[0].normal = { 0.0f, 0.0f, -1.0f };
	//vertexData_[1].normal = { 0.0f, 0.0f, -1.0f };
	//vertexData_[2].normal = { 0.0f, 0.0f, -1.0f };
	//vertexData_[3].normal = { 0.0f, 0.0f, -1.0f };

	// VertexBufferView
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * kVertexCount);
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
	ID3D12GraphicsCommandList* commandList = object3dCommon->GetDxCommon()->GetCommandList();

	// =====================================
   // 1. VertexBufferViewを設定
   // =====================================
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// =====================================
   // 2. 座標変換行列CBufの場所を設定
   // RootParameter[1]
   // =====================================
	commandList->SetGraphicsRootConstantBufferView(
		0,
		transformationMatrixResource_->GetGPUVirtualAddress()
	);

	// =====================================
   // 3. マテリアルCBufの場所を設定
   // RootParameter[0]
   // =====================================
	commandList->SetGraphicsRootConstantBufferView(
		1,
		materialResource_->GetGPUVirtualAddress()
	);
	// =====================================
	// 4. SRV DescriptorTableの先頭を設定
	// RootParameter[2]
	// =====================================
	uint32_t textureIndex =
		TextureManager::GetInstance()->GetTextureIndexByFilePath(
			modelData.material.textureFilePath
		);

	commandList->SetGraphicsRootDescriptorTable(
		2,
		TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex)
	);

	// =====================================
	// 5. 平行光源Bufferの場所を設定
	// RootParameter[3]
	// =====================================
	commandList->SetGraphicsRootConstantBufferView(
		3,
		directionalLightResource_->GetGPUVirtualAddress()
	);

	// =====================================
	// 6. 描画 (DrawCall)
	// =====================================
	commandList->DrawInstanced(
		static_cast<UINT>(modelData.vertices.size()),
		1,
		0,
		0
	);

}

