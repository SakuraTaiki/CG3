#include "SceneObjectController.h"

#include "ModelManager.h"
#include "Object3dCommon.h"

void SceneObjectController::Initialize(
    Object3dCommon* object3dCommon,
    uint32_t environmentTextureHandle,
    float environmentCoefficient
) {
    Model* modelPlane =
        ModelManager::Load("Resources/terrain", "terrain.obj");

    Model* modelAxis =
        ModelManager::Load("axis.obj");

    {
        std::unique_ptr<Object3d> object =
            std::make_unique<Object3d>();

        object->Initialize(object3dCommon);
        object->SetModel(modelPlane);
        object->SetPosition({ 0.0f, 1.0f, 0.0f });
        object->SetRotation({ 0.0f, 0.0f, 0.0f });
        object->SetScale({ 0.5f, 0.5f, 0.5f });

        object->SetEnvironmentTexture(environmentTextureHandle);
        object->SetEnvironmentCoefficient(environmentCoefficient);

        objects_.push_back(std::move(object));
    }

    {
        std::unique_ptr<Object3d> object =
            std::make_unique<Object3d>();

        object->Initialize(object3dCommon);
        object->SetModel(modelAxis);
        object->SetPosition({ 2.0f, 0.0f, 0.0f });
        object->SetRotation({ 1.57f, 0.0f, 0.0f });

        object->SetEnvironmentTexture(environmentTextureHandle);
        object->SetEnvironmentCoefficient(environmentCoefficient);

        objects_.push_back(std::move(object));
    }

    {
        std::unique_ptr<Object3d> object =
            std::make_unique<Object3d>();

        object->Initialize(object3dCommon);
        object->SetModel(modelAxis);
        object->SetPosition({ -2.0f, 0.0f, 0.0f });
        object->SetRotation({ 1.57f, 0.0f, 0.0f });

        object->SetEnvironmentTexture(environmentTextureHandle);
        object->SetEnvironmentCoefficient(environmentCoefficient);

        objects_.push_back(std::move(object));
    }
}

void SceneObjectController::Finalize() {
    objects_.clear();
}

void SceneObjectController::Update() {
    for (auto& object : objects_) {
        object->Update();
    }
}

void SceneObjectController::Draw() {
    for (size_t index = 0; index < objects_.size(); ++index) {
        if (index == 0 && !showTerrain_) {
            continue;
        }

        if (index != 0 && !showAxis_) {
            continue;
        }

        objects_[index]->Draw();
    }
}

void SceneObjectController::SetEnvironmentCoefficient(
    float environmentCoefficient
) {
    for (auto& object : objects_) {
        object->SetEnvironmentCoefficient(environmentCoefficient);
    }
}