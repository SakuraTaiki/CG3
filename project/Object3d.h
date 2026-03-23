#pragma once
#include "Math.h"

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <vector>

class Sprite;
class Object3dCommon;
class Object3d
{
public://メンバ関数

	Object3dCommon* object3dCommon = nullptr;
	void Initialize(Object3dCommon*object3dCommon);
	void Update();
	void Draw();
private:

	struct VertexData
	{
		Math::Vector4 position;
		Math::Vector2 texcoord;
		Math::Vector3 normal; // スライドの指示通り、一旦normalも含めておく
	};

	//=========Vertex
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	VertexData* vertexData_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	void CreateVertexData();

	struct MaterialData
	{
		std::string textureFilePath;
		uint32_t textureIndex = 0;
	};

	struct ModelData
	{
		std::vector<VertexData>vertices;
		MaterialData material;
	};
	ModelData modelData;

	struct Material
	{
		Math::Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Math::Matrix4x4 uvTransform;
	};

	// ===== Material
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Material* materialData_ = nullptr;

	void CreateMaterial();

	struct TransformationMatrix {
		Math::Matrix4x4 WVP;    // World View Projection Matrix
		Math::Matrix4x4 World;  // World Matrix
	};

	// ===== TransformationMatrix
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
	TransformationMatrix* transformationMatrixData_ = nullptr;

	Math::Transform transform_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	void CreateTransformationMatrix();

	void UpdateTransformationMatrix();

	struct DirectionalLight
	{
		Math::Vector4 color; // xyz = color, w = intensity
		Math::Vector3 direction;  // 向き
		float padding;            // 16byte alignment
	};

	Microsoft::WRL::ComPtr<ID3D12Resource>directionalLightResource_;
	DirectionalLight* directionalLightData_ = nullptr;

	void CreateDirectionalLight();

	Math::Transform transform;
	Math::Transform cameraTransform;

	//mtlファイルの読み取り
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	//objファイル読み取り
	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

	
};

