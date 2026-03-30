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

void ModelManager::Initialize(ModelCommon* modelCommon) {
    modelCommon_ = modelCommon;
}
