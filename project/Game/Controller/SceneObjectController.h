#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <string>

#include "Object3d.h"

class Object3dCommon;
class Model;

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

    size_t AddEditorObject(Model* model, const std::string& name);

    size_t GetObjectCount() const;
    Object3d* GetObject(size_t index);
    const Object3d* GetObject(size_t index) const;
    const std::string& GetObjectName(size_t index) const;
    bool IsObjectVisible(size_t index) const;
    void SetObjectVisible(size_t index, bool visible);

    static size_t GetEditorObjectIndex(EditorObjectType type);

    bool& ShowTerrain() { return showTerrain_; }
    bool& ShowAxis() { return showAxis_; }

private:
    Object3dCommon* object3dCommon_ = nullptr;
    uint32_t environmentTextureHandle_ = 0;
    float environmentCoefficient_ = 0.05f;

    std::vector<std::unique_ptr<Object3d>> objects_;
    std::vector<std::string> objectNames_;
    std::vector<uint8_t> objectVisible_;

    bool showTerrain_ = true;
    bool showAxis_ = true;
};
