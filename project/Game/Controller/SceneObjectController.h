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
    size_t AddEditorObject(
        Model* model,
        const std::string& name,
        const std::string& modelPath
    );

    void ClearEditorObjects();

    size_t GetObjectCount() const;
    Object3d* GetObject(size_t index);
    const Object3d* GetObject(size_t index) const;
    const std::string& GetObjectName(size_t index) const;
    bool IsObjectVisible(size_t index) const;
    void SetObjectVisible(size_t index, bool visible);

    void SetObjectModelPath(size_t index, const std::string& path);
    const std::string& GetObjectModelPath(size_t index) const;

    void SetObjectTexturePath(size_t index, const std::string& path);
    const std::string& GetObjectTexturePath(size_t index) const;

    bool SaveEditorSceneToJson(const std::string& filePath) const;
    bool LoadEditorSceneFromJson(const std::string& filePath);

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
    std::vector<std::string> objectModelPaths_;
    std::vector<std::string> objectTexturePaths_;

    bool showTerrain_ = true;
    bool showAxis_ = true;
};
