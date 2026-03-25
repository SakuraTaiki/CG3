#include"Model.h"
#include"TextureManager.h"
#include"ModelCommon.h"

#include <fstream>
#include <sstream>
#include <cassert>

void Model::CreateVertexBuffer()
{
    auto device = modelCommon_->GetDxCommon()->GetDevice();

    const size_t size = sizeof(VertexData) * modelData_.vertices.size();

    vertexResource_ =
        modelCommon_->GetDxCommon()->CreateBufferResource(size);

    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

    std::copy(
        modelData_.vertices.begin(),
        modelData_.vertices.end(),
        vertexData_
    );

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(size);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
}

void Model::CreateMaterial()
{
    materialResource_ =
        modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));

    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    materialData_->color = { 1,1,1,1 };
    materialData_->enableLighting = false;
    materialData_->uvTransform = Math::MakeIdentity4x4();
}

Model::MaterialData Model::LoadMaterialTemplateFile(
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

Model::ModelData Model::LoadObjFile(
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
            tex.y = 1.0f - tex.y; // 上下反転
            texcoords.push_back(tex);
        }

        // 法線
        else if (identifier == "vn") {
            Math::Vector3 normal;
            s >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }

        // 面（三角形前提）
        else if (identifier == "f") {
            std::string vertexStr;

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

        // マテリアル
        else if (identifier == "mtllib") {
            std::string mtlFile;
            s >> mtlFile;
            modelData.material =
                LoadMaterialTemplateFile(directoryPath, mtlFile);
        }
    }

    return modelData;
}

void Model::Initialize(ModelCommon* modelCommon, const std::string& dir, const std::string& file) 
{
    modelCommon_ = modelCommon;

    modelData_ = LoadObjFile(dir, file);

    // テクスチャ読み込み
    TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilePath);

    CreateVertexBuffer();
    CreateMaterial();
}

void Model::Draw() 
{
    ID3D12GraphicsCommandList* commandList =
        modelCommon_->GetDxCommon()->GetCommandList();

 // =====================================
 // 1. VertexBufferViewを設定
 // =====================================
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);


    // =====================================
    // 2. マテリアルCBufferの場所を設定
    // RootParameter[1]
    // =====================================
    commandList->SetGraphicsRootConstantBufferView(
        1,
        materialResource_->GetGPUVirtualAddress()
    );

  // =====================================
  // 3. SRV DescriptorTableの先頭を設定
  // RootParameter[2]
  // =====================================
    uint32_t textureIndex =
        TextureManager::GetInstance()->GetTextureIndexByFilePath(
            modelData_.material.textureFilePath
        );

    commandList->SetGraphicsRootDescriptorTable(
        2,
        TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex)
    );


     // =====================================
    // 4. 描画（DrawCall）
    // =====================================
    commandList->DrawInstanced(
        static_cast<UINT>(modelData_.vertices.size()),
        1,
        0,
        0
    );

}