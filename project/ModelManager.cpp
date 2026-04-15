#include "ModelManager.h"
#include"ModelCommon.h"

ModelManager* ModelManager::instance_ = nullptr;

ModelManager* ModelManager::GetInstance() {
    if (!instance_) {
        instance_ = new ModelManager();
    }
    return instance_;
}

void ModelManager::Initialize(DirectXCommon*dxCommon) {
	modelCommon = new ModelCommon;
	modelCommon->Intialize(dxCommon);
}

void ModelManager::LoadModel(const std::string& filePath)
{
	if (models_.contains(filePath)) {
		return;
	}

    std::unique_ptr<Model>model = std::make_unique<Model>();

    model->Initialize(modelCommon, "resources", filePath);

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

void ModelManager::Finalize() {
    models_.clear();

    delete modelCommon;
    modelCommon = nullptr;
}
