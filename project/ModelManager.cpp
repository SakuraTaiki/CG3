#include "ModelManager.h"
#include"ModelCommon.h"

ModelManager* ModelManager::instance = nullptr;

//�C���X�^���X�擾
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
	std::unique_ptr<Model>model = std::make_unique<Model>();
	model->Initialize(modelCommon, "resources", filePath);

	models.insert(std::make_pair(filePath, std::move(model)));

	if (models.contains(filePath)) {
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
	if (models.contains(filePath)) {
		return models.at(filePath).get();
	}
	return nullptr;
}
