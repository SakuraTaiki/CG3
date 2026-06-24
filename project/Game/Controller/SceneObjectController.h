#pragma once

#include <memory>
#include <vector>
#include <cstdint>

#include "Object3d.h"

class Object3dCommon;

class SceneObjectController {
public:
    void Initialize(
        Object3dCommon* object3dCommon,
        uint32_t environmentTextureHandle,
        float environmentCoefficient
    );

    void Finalize();

    void Update();
    void Draw();

    void SetEnvironmentCoefficient(float environmentCoefficient);

private:
    std::vector<std::unique_ptr<Object3d>> objects_;
};