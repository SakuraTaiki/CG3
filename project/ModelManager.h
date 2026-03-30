#pragma once
#include <unordered_map>
#include <string>
#include "Model.h"

class ModelCommon;

class ModelManager
{
public:
    // ★ インスタンス取得
    static ModelManager* GetInstance();

    // 初期化
    void Initialize(ModelCommon* modelCommon);

    // モデル読み込み
    Model* LoadModel(const std::string& dir, const std::string& file);

private:
    ModelManager() = default;
    ~ModelManager() = default;

    // コピー禁止（シングルトン）
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    void Finalize();

private:
    static ModelManager* instance_;

    ModelCommon* modelCommon_ = nullptr;

    // ★ モデル管理
    std::unordered_map<std::string, Model*> models_;

    
};

