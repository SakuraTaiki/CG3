#include "Model.h"
#include <fstream>
#include <sstream>
#include <cassert>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>

using namespace Microsoft::WRL;

Model* Model::CreateFromOBJ(DirectXCommon* dxCommon, const std::string& directoryPath, const std::string& filename, TextureManager* textureManager) {
    Model* model = new Model();
    model->Initialize(dxCommon, directoryPath, filename, textureManager);
    return model;
}

void Model::Initialize(DirectXCommon* dxCommon, const std::string& directoryPath, const std::string& filename, TextureManager* textureManager) {
    // 1. OBJ読み込み
    LoadObjFile(directoryPath, filename);

    // ★安全策：頂点が一つもない場合はバッファを作らない
    if (vertices_.empty()) {
        OutputDebugStringA("Error: Model vertices are empty!\n");
        return;
    }

    // 2. バッファ生成
    CreateBuffers(dxCommon);

    if (textureManager) {
        if (!textureFilePath_.empty()) {
            textureHandle_ = textureManager->LoadTexture(textureFilePath_);
        } else {
            textureHandle_ = textureManager->LoadTexture("Resources/uvChecker.png");
        }
    }
}

void Model::LoadObjFile(const std::string& directoryPath,
    const std::string& filename)
{
    vertices_.clear();

    Assimp::Importer importer;

    std::string filePath = directoryPath + "/" + filename;

    const aiScene* scene = importer.ReadFile(
        filePath.c_str(),
        aiProcess_FlipWindingOrder |
        aiProcess_FlipUVs |
        aiProcess_Triangulate
    );

    assert(scene->HasMeshes());

    //------------------------------------
    // Mesh解析
    //------------------------------------
    for (uint32_t meshIndex = 0;
        meshIndex < scene->mNumMeshes;
        ++meshIndex)
    {
        aiMesh* mesh = scene->mMeshes[meshIndex];

        assert(mesh->HasNormals());

        //--------------------------------
        // Face解析
        //--------------------------------
        for (uint32_t faceIndex = 0;
            faceIndex < mesh->mNumFaces;
            ++faceIndex)
        {
            aiFace& face = mesh->mFaces[faceIndex];

            assert(face.mNumIndices == 3);

            //--------------------------------
            // Vertex解析
            //--------------------------------
            for (uint32_t element = 0;
                element < face.mNumIndices;
                ++element)
            {
                uint32_t vertexIndex = face.mIndices[element];

                aiVector3D position = mesh->mVertices[vertexIndex];
                aiVector3D normal = mesh->mNormals[vertexIndex];

                ModelVertexData vertex{};
                vertex.position = {
                    position.x,
                    position.y,
                    position.z,
                    1.0f
                };

                vertex.normal = {
                    normal.x,
                    normal.y,
                    normal.z
                };

                // UVがある場合
                if (mesh->HasTextureCoords(0)) {
                    aiVector3D texcoord = mesh->mTextureCoords[0][vertexIndex];
                    vertex.texcoord = {
                        texcoord.x,
                        texcoord.y
                    };
                }
                // UVが無い場合：位置から仮UVを作る
                else {
                    vertex.texcoord = {
                        position.x,
                        position.z
                    };
                }

                // 左手系変換
                vertex.position.x *= -1.0f;
                vertex.normal.x *= -1.0f;

                vertices_.push_back(vertex);
            }
        }
    }

    OutputDebugStringA(
        ("Loaded : " + filename +
            " VertexCount = " +
            std::to_string(vertices_.size()) + "\n").c_str());

    // ------------------------------------
    // Material解析
    // ------------------------------------
    textureFilePath_.clear();

    for (uint32_t materialIndex = 0;
        materialIndex < scene->mNumMaterials;
        ++materialIndex)
    {
        aiMaterial* material = scene->mMaterials[materialIndex];

        if (material->GetTextureCount(aiTextureType_DIFFUSE) == 0) {
            continue;
        }

        aiString textureFilePath;
        material->GetTexture(
            aiTextureType_DIFFUSE,
            0,
            &textureFilePath
        );

        std::string path = directoryPath + "/" + textureFilePath.C_Str();

        // パス区切りをWindowsでも安全にする
        std::replace(path.begin(), path.end(), '\\', '/');

        // 実在するテクスチャだけ採用する
        if (std::filesystem::exists(path)) {
            textureFilePath_ = path;
            break;
        }
    }
}

void Model::CreateBuffers(DirectXCommon* dxCommon) {
    auto device = dxCommon->GetDevice();
    UINT sizeIB = static_cast<UINT>(sizeof(ModelVertexData) * vertices_.size());

    D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = sizeIB; // ここが0だとCreateCommittedResourceは失敗する
    resDesc.Height = 1; resDesc.DepthOrArraySize = 1; resDesc.MipLevels = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; resDesc.SampleDesc.Count = 1;

    // ★重要：HRESULTで成功を確認する
    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer_));

    if (FAILED(hr)) {
        assert(false && "Failed to create Vertex Buffer");
        return;
    }

    // データ転送
    ModelVertexData* vertMap = nullptr;
    hr = vertexBuffer_->Map(0, nullptr, (void**)&vertMap);
    if (SUCCEEDED(hr)) {
        std::copy(vertices_.begin(), vertices_.end(), vertMap);
        vertexBuffer_->Unmap(0, nullptr);
    }

    // View作成
    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeIB;
    vertexBufferView_.StrideInBytes = sizeof(ModelVertexData);
}
void Model::Draw(ID3D12GraphicsCommandList* commandList) {
    // ★安全策：バッファがない場合は描画しない
    if (!vertexBuffer_) {
        return;
    }

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->DrawInstanced(UINT(vertices_.size()), 1, 0, 0);
}