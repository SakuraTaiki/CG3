#pragma once
#include"Math.h"
#include "Object3d.h"


class Object3dManager
{
public:
    void Initialize(Object3dCommon* common, ModelCommon* modelCommon);

    void Update();
    void Draw();

    void CreateObject(const Math::Vector3& pos);

private:
    std::vector<Object3d*> objects_;
    Object3dCommon* common_ = nullptr;
};

