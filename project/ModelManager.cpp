#include "ModelManager.h"
#include"ModelCommon.h"

ModelManager* ModelManager::instance_ = nullptr;

ModelManager* ModelManager::GetInstance() {
    if (!instance_) {
        instance_ = new ModelManager();
    }
    return instance_;
}

void ModelManager::Initialize(DirectXCommon* dxCommon) {
    modelCommon_ = new ModelCommon;
}

void ModelManager::Finalize() {
    models_.clear();

    delete modelCommon_;
    modelCommon_ = nullptr;

    delete instance_;
    instance_ = nullptr;
}


void ModelManager::LoadModel(const std::string& filePath)
{
    // すでにあるかチェック
    auto it = models_.find(filePath);
    if (it != models_.end()) {
        return it->second.get();
    }



    std::unique_ptr<Model>model = std::make_unique<Model>();

    std::string directory = "resources/" + filePath.substr(0, filePath.find_last_of('/'));
    std::string filename = filePath.substr(filePath.find_last_of('/') + 1);


    model->Initialize(modelCommon_, "resources", filePath);

    models_.insert(std::make_pair(filePath, std::move(model)));
}


Model* ModelManager::FindModel(const std::string& filePath)
{
    if (models_.contains(filePath)) {
        //読み込みモデルを戻り値としてreturn
        return models_.at(filePath).get();
    }
	return nullptr;
}

