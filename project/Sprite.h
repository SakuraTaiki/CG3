#pragma once
#include "Mymath.h"
class SpriteCommon;
class Sprite
{
public:
	
	void Initialize(SpriteCommon* spriteCommon);
	void Draw();


	struct VertexData
	{
		MyMath::Vector4 position;
		MyMath::Vector2 texcoord;
		MyMath::Vector3 normal;
	};

	struct Material
	{
		MyMath::Vector4 color;
		int32_t enableLighting;
		float padding[3];
		MyMath::Matrix4x4 uvTransform;
	};

private:

	SpriteCommon* spriteCommon_ = nullptr;

	//=======Vertex
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	VertexData* vertexData_ = nullptr;
	uint32_t* indexData = nullptr;

	//=======Index
	Microsoft::WRL::ComPtr<ID3D12Resource>indexResource_;
	uint32_t* indexData_ = nullptr;
	

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

private:
	void CreateVertexResource();
	void CreateIndexResource();
	void CreateVertexBufferView();
	void CreateIndexBufferView();
};

