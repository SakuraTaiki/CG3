#pragma once
#include <map>
#include <memory>
#include <string>
#include "Model.h"

class DirectXCommon;
class ModelCommon;

class ModelManager
{
public:

    static ModelManager* GetInstance();

    // 初期化
    void Initialize(DirectXCommon*dxCommon);

    // モデル読み込み
    void LoadModel(const std::string&filePath);

    void Finalize();

    Model* FindModel(const std::string& filePath);
private:
	ModelManager() = default;

	// デストラクタ
	~ModelManager() = default;

    // コピー禁止（シングルトン）
	ModelManager(const ModelManager&) = delete;

	// コピー代入演算子（禁止）
	ModelManager& operator=(const ModelManager&) = delete;

private:
    static ModelManager* instance_;

    ModelCommon* modelCommon_ = nullptr;

    // ★ モデル管理
    std::map<std::string, std::unique_ptr<Model>>models_;
};

