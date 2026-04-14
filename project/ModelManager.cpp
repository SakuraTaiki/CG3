#include "ModelManager.h"
#include "ModelCommon.h"

ModelManager* ModelManager::instance_ = nullptr;

//インスタンス取得
ModelManager* ModelManager::GetInstance() {
    if (!instance_) {
        instance_ = new ModelManager();
    }
    return instance_;
}

void ModelManager::Initialize(DirectXCommon*dxCommon) {
    modelCommon_ = new ModelCommon;
    modelCommon_->Intialize(dxCommon);
}



void ModelManager::LoadModel(const std::string& filePath)
{
    if (models_.contains(filePath)) {
        //読み込み済みなら早期リターン
        return;
    }

    //モデル生成
    std::unique_ptr<Model>model = std::make_unique<Model>();

    //モデル読み込み
    model->Initialize(modelCommon_, "resources",filePath);

    //モデルをmapコンテナに格納する
    models_.insert(std::make_pair(filePath, std::move(model)));

}

void ModelManager::Finalize() {
    if (!instance_) {
        return;
    }
    instance_->models_.clear();

    delete instance_->modelCommon_;
    instance_->modelCommon_ = nullptr;

    delete instance_;
    instance_ = nullptr;
}

Model* ModelManager::FindModel(const std::string& filePath)
{
    if (models_.contains(filePath)) {
        //読み込みモデルを戻り値としてreturn
        return models_.at(filePath).get();
    }
    return nullptr;
}
