#include "SceneObjectController.h"

#include "ModelManager.h"
#include "Object3dCommon.h"

void SceneObjectController::Initialize(
    Object3dCommon* object3dCommon,
    uint32_t environmentTextureHandle,
    float environmentCoefficient
) {
    object3dCommon_ = object3dCommon;
    environmentTextureHandle_ = environmentTextureHandle;
    environmentCoefficient_ = environmentCoefficient;

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
        objectNames_.push_back("Terrain");
        objectVisible_.push_back(1);
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
        objectNames_.push_back("Axis +X");
        objectVisible_.push_back(1);
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
        objectNames_.push_back("Axis -X");
        objectVisible_.push_back(1);
    }
}

void SceneObjectController::Finalize() {
    objects_.clear();
    objectNames_.clear();
    objectVisible_.clear();
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

        if ((index == 1 || index == 2) && !showAxis_) {
            continue;
        }

        if (index >= 3 && index < objectVisible_.size() && !objectVisible_[index]) {
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

Object3d* SceneObjectController::GetEditorObject(EditorObjectType type)
{
    return GetObject(GetEditorObjectIndex(type));
}

const Object3d* SceneObjectController::GetEditorObject(EditorObjectType type) const
{
    return GetObject(GetEditorObjectIndex(type));
}

size_t SceneObjectController::AddEditorObject(Model* model, const std::string& name)
{
    if (!object3dCommon_ || !model) {
        return static_cast<size_t>(-1);
    }

    std::unique_ptr<Object3d> object =
        std::make_unique<Object3d>();

    object->Initialize(object3dCommon_);
    object->SetModel(model);
    object->SetPosition({ 0.0f, 0.0f, 0.0f });
    object->SetRotation({ 0.0f, 0.0f, 0.0f });
    object->SetScale({ 1.0f, 1.0f, 1.0f });
    object->SetEnvironmentTexture(environmentTextureHandle_);
    object->SetEnvironmentCoefficient(environmentCoefficient_);

    const size_t index = objects_.size();

    objects_.push_back(std::move(object));
    objectNames_.push_back(name);
    objectVisible_.push_back(1);

    return index;
}

size_t SceneObjectController::GetObjectCount() const
{
    return objects_.size();
}

Object3d* SceneObjectController::GetObject(size_t index)
{
    if (index >= objects_.size()) {
        return nullptr;
    }

    return objects_[index].get();
}

const Object3d* SceneObjectController::GetObject(size_t index) const
{
    if (index >= objects_.size()) {
        return nullptr;
    }

    return objects_[index].get();
}

const std::string& SceneObjectController::GetObjectName(size_t index) const
{
    static const std::string kEmptyName = "";

    if (index >= objectNames_.size()) {
        return kEmptyName;
    }

    return objectNames_[index];
}

bool SceneObjectController::IsObjectVisible(size_t index) const
{
    if (index == 0) {
        return showTerrain_;
    }

    if (index == 1 || index == 2) {
        return showAxis_;
    }

    if (index >= objectVisible_.size()) {
        return false;
    }

    return objectVisible_[index] != 0;
}

void SceneObjectController::SetObjectVisible(size_t index, bool visible)
{
    if (index == 0) {
        showTerrain_ = visible;
        return;
    }

    if (index == 1 || index == 2) {
        showAxis_ = visible;
        return;
    }

    if (index >= objectVisible_.size()) {
        return;
    }

    objectVisible_[index] = visible ? 1 : 0;
}

size_t SceneObjectController::GetEditorObjectIndex(EditorObjectType type)
{
    switch (type) {
    case EditorObjectType::Terrain:
        return 0;

    case EditorObjectType::AxisPositive:
        return 1;

    case EditorObjectType::AxisNegative:
        return 2;

    default:
        return 0;
    }
}
