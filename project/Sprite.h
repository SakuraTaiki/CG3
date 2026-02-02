#pragma once
#include "Math.h"
#include <wrl.h>
#include <d3d12.h>
#include <string>

class SpriteCommon;

class Sprite {
public:

    uint32_t textureIndex = 0;

    void Initialize(SpriteCommon* spriteCommon,std::string textureFilePath );
    void Update();
    void Draw(ID3D12GraphicsCommandList* commandList);

    const Math::Vector2& GetPosition() const { return position_; }

    void SetPosition(const Math::Vector2& position) { this->position_ = position; }

    const Math::Vector3& GetScale() const { return transform_.scale; }

    void SetScale(const Math::Vector3& scale) { this->transform_.scale = scale; }

    const Math::Vector3& GetRotation() const { return transform_.rotate; }

    void SetRotation(const Math::Vector3& rotate) { this->transform_.rotate = rotate; }

    const Math::Vector4& GetColor() const { return materialData_->color; }

    void SetColor(const Math::Vector4& color) { this->materialData_->color = color; }

    const Math::Vector2& GetSize() const { return size_; }

    void SetSize(const Math::Vector2& size) { this->size_ = size; }

    const Math::Vector2& GetAnchorPoint() const { return anchorPoint_; }

    void SetAnchorPoint(const Math::Vector2& anchorPoint) { this->anchorPoint_ = anchorPoint; }

private:
    SpriteCommon* spriteCommon_ = nullptr;

    struct VertexData {
        Math::Vector4 position;
        Math::Vector2 texcoord;
        Math::Vector3 normal; // スライドの指示通り、一旦normalも含めておく
    };


    // ===== Vertex
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    VertexData* vertexData_ = nullptr;

    // ===== Index
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    uint32_t* indexData_ = nullptr;

  
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

    void CreateVertexData();

    struct Material
    {
     Math::Vector4 color;
     int32_t enableLighting;
     float padding[3];                // パディング (float[3])
     Math::Matrix4x4 uvTransform;     // UV変換行列 (Matrix4x4)
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

    Math::Vector2 position_ = { 500.0f, 400.0f };

    float rotation = 0.0f;

    Math::Vector2 size_ = { 100.0f, 100.0f };

    Math::Vector2 anchorPoint_ = { 0.5f, 0.5f };
};