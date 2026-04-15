#include "Object3dManager.h"
void Object3dManager::Initialize(Object3dCommon* common, ModelCommon* modelCommon)
{
    common_ = common;

}

void Object3dManager::CreateObject(const Math::Vector3& pos)
{
    Object3d* obj = new Object3d();
    obj->Initialize(common_);

    obj->transform.translate = pos;

    objects_.push_back(obj);
}

void Object3dManager::Update()
{
    for (auto obj : objects_) {
        obj->Update();
    }
}

void Object3dManager::Draw()
{
    for (auto obj : objects_) {
        obj->Draw();
    }
}