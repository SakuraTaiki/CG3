#include "ModelManager.h"
#include <cassert>

Object3dCommon* ModelManager::common_ = nullptr;
std::unordered_map<std::string, std::unique_ptr<Model>> ModelManager::models_;

void ModelManager::Initialize(Object3dCommon* common) {
    assert(common);
    common_ = common;
}

void ModelManager::Finalize() {
    models_.clear();
    common_ = nullptr;
}

Model* ModelManager::Load(const std::string& modelName) {
    auto it = models_.find(modelName);
    if (it != models_.end()) {
        return it->second.get();
    }

    std::unique_ptr<Model> model(
        Model::CreateFromOBJ(
            common_->GetDxCommon(),
            "Resources",
            modelName,
            common_->GetTextureManager()
        )
    );

    Model* result = model.get();
    models_[modelName] = std::move(model);

    return result;
}

Model* ModelManager::Load(const std::string& directoryPath, const std::string& modelName)
{
    std::string key = directoryPath + "/" + modelName;

    auto it = models_.find(key);
    if (it != models_.end()) {
        return it->second.get();
    }

    std::unique_ptr<Model> model(
        Model::CreateFromOBJ(
            common_->GetDxCommon(),
            directoryPath,
            modelName,
            common_->GetTextureManager()
        )
    );

    Model* result = model.get();
    models_[key] = std::move(model);

    return result;
}

Model* ModelManager::Find(const std::string& modelName) {
    auto it = models_.find(modelName);
    if (it == models_.end()) {
        return nullptr;
    }

    return it->second.get();
}