#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <list>
#include "MyMath.h"
#include "DirectXCommon.h"
#include "TextureManager.h"

struct Particle
{
	Transform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float maxTime;
};

class ParticleManager
{
public:
	// 定数：最大パーティクル数
	static const uint32_t kMaxParticles = 1024;

	struct InstanceData {
		Matrix4x4 WVP;
		Vector4 color;
	};

	struct VertexData
	{
		Vector4 position;
		Vector3 texcoord;
		Vector2 normal;
	};

	void Initialize(DirectXCommon* dxCommon, TextureManager* textureManager);
	void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);
	void Draw();
	void Emit(const Vector3&pos,uint32_t count);

private:

	void CreateRootSignature();
	void CreatePipelineState();
	void CreateMesh();

private:

	DirectXCommon* dxCommon_ = nullptr;
	TextureManager* textureManager_ = nullptr;

	// DirectXリソース
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

	// モデルデータ（板ポリ）
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	// インスタンシング用データ
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingBuffer_;
	D3D12_VERTEX_BUFFER_VIEW instancingBufferView_{};
	InstanceData* instancingDataMapped_ = nullptr;

	// テクスチャハンドル
	uint32_t textureHandle_ = 0;

	// パーティクルリスト
	std::list<Particle> particles_;

};

