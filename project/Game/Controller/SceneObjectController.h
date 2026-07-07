#pragma once

#include <memory>
#include <vector>
#include <cstdint>

#include "Object3d.h"

class Object3dCommon;

class SceneObjectController {
public:

    enum class EditorObjectType {
        Terrain,
        AxisPositive,
        AxisNegative
    };


    void Initialize(
        Object3dCommon* object3dCommon,
        uint32_t environmentTextureHandle,
        float environmentCoefficient
    );

    void Finalize();

    void Update();
    void Draw();

    void SetEnvironmentCoefficient(float environmentCoefficient);

    Object3d* GetEditorObject(EditorObjectType type);
    const Object3d* GetEditorObject(EditorObjectType type) const;

    bool& ShowTerrain() { return showTerrain_; }
    bool& ShowAxis() { return showAxis_; }

private:
    std::vector<std::unique_ptr<Object3d>> objects_;
    bool showTerrain_ = true;
    bool showAxis_ = true;
};