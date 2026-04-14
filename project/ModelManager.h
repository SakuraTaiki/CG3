#pragma once
#include <string>
#include "Model.h"
#include <map>
#include<memory>

class DirectXCommon;

class ModelManager
{
public:
    // ★ インスタンス取得
    static ModelManager* GetInstance();

    // 初期化
    void Initialize(DirectXCommon*dxCommon);

    // モデル読み込み
    void LoadModel(const std::string&filePath);

    void Finalize();

    Model* FindModel(const std::string& filePath);
private:
    ModelManager() = default;
    ~ModelManager() = default;

    // コピー禁止（シングルトン）
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

   

private:
    static ModelManager* instance_;

    ModelCommon* modelCommon_ = nullptr;

    // ★ モデル管理
    std::map<std::string, std::unique_ptr<Model>>models_;
};

