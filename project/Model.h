#pragma once
#include "Math.h"

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <vector>

//3Dモデル
class ModelCommon;
class Model 
{
public://メンバ
	void Initialize(ModelCommon* modelCommon,const std::string& dir, const std::string& file);
	void Update();
	void Draw();

private:
	ModelCommon* modelCommon_ = nullptr;

	// =========================
	// 頂点データ
	// =========================
	struct VertexData {
		Math::Vector4 position;
		Math::Vector2 texcoord;
		Math::Vector3 normal;
	};

	std::vector<VertexData> vertices_;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	VertexData* vertexData_ = nullptr;

	void CreateVertexBuffer();

	// =========================
	// マテリアル
	// =========================
	struct Material {
		Math::Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Math::Matrix4x4 uvTransform;
	};

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Material* materialData_ = nullptr;

	void CreateMaterial();

	// =========================
	// OBJデータ
	// =========================
	struct MaterialData {
		std::string textureFilePath;
		uint32_t textureIndex = 0;
	};

	struct ModelData {
		std::vector<VertexData> vertices;
		MaterialData material;
	};

	ModelData modelData_;

	// =========================
	// 読み込み
	// =========================
	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

};
