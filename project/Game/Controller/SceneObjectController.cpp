#include "SceneObjectController.h"

#include "ModelManager.h"
#include "Object3dCommon.h"
#include "TextureManager.h"
#include "externals/json/json.hpp"

#include <fstream>
#include <regex>
#include <sstream>

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
        objectModelPaths_.push_back("Resources/terrain/terrain.obj");
        objectTexturePaths_.push_back("");
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
        objectModelPaths_.push_back("axis.obj");
        objectTexturePaths_.push_back("");
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
        objectModelPaths_.push_back("axis.obj");
        objectTexturePaths_.push_back("");
    }
}

void SceneObjectController::Finalize() {
    objects_.clear();
    objectNames_.clear();
    objectVisible_.clear();
    objectModelPaths_.clear();
    objectTexturePaths_.clear();
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
    return AddEditorObject(model, name, "");
}

size_t SceneObjectController::AddEditorObject(
    Model* model,
    const std::string& name,
    const std::string& modelPath
)
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
    objectModelPaths_.push_back(modelPath);
    objectTexturePaths_.push_back("");

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

void SceneObjectController::ClearEditorObjects()
{
    if (objects_.size() <= 3) {
        return;
    }

    objects_.resize(3);
    objectNames_.resize(3);
    objectVisible_.resize(3);
    objectModelPaths_.resize(3);
    objectTexturePaths_.resize(3);
}

void SceneObjectController::SetObjectModelPath(
    size_t index,
    const std::string& path
)
{
    if (index >= objectModelPaths_.size()) {
        return;
    }

    objectModelPaths_[index] = path;
}

const std::string& SceneObjectController::GetObjectModelPath(size_t index) const
{
    static const std::string kEmpty = "";

    if (index >= objectModelPaths_.size()) {
        return kEmpty;
    }

    return objectModelPaths_[index];
}

void SceneObjectController::SetObjectTexturePath(
    size_t index,
    const std::string& path
)
{
    if (index >= objectTexturePaths_.size()) {
        return;
    }

    objectTexturePaths_[index] = path;
}

const std::string& SceneObjectController::GetObjectTexturePath(size_t index) const
{
    static const std::string kEmpty = "";

    if (index >= objectTexturePaths_.size()) {
        return kEmpty;
    }

    return objectTexturePaths_[index];
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

namespace {

    std::string EscapeJsonString(const std::string& value)
    {
        std::string result;

        for (char c : value) {
            if (c == '\\') {
                result += "\\\\";
            } else if (c == '"') {
                result += "\\\"";
            } else {
                result += c;
            }
        }

        return result;
    }

    bool SplitModelPath(
        const std::string& path,
        std::string& directoryPath,
        std::string& modelName
    )
    {
        const size_t slashPos =
            path.find_last_of("/\\");

        if (slashPos == std::string::npos) {
            directoryPath = "Resources";
            modelName = path;
            return true;
        }

        directoryPath =
            path.substr(0, slashPos);

        modelName =
            path.substr(slashPos + 1);

        return true;
    }

    std::string ExtractJsonString(
        const std::string& text,
        const std::string& key
    )
    {
        const std::regex pattern(
            "\"" + key + "\"\\s*:\\s*\"([^\"]*)\""
        );

        std::smatch match;

        if (std::regex_search(text, match, pattern)) {
            return match[1].str();
        }

        return "";
    }

    bool ExtractJsonBool(
        const std::string& text,
        const std::string& key,
        bool defaultValue
    )
    {
        const std::regex pattern(
            "\"" + key + "\"\\s*:\\s*(true|false)"
        );

        std::smatch match;

        if (std::regex_search(text, match, pattern)) {
            return match[1].str() == "true";
        }

        return defaultValue;
    }

    Vector3 ExtractJsonVector3(
        const std::string& text,
        const std::string& key,
        const Vector3& defaultValue
    )
    {
        const std::regex pattern(
            "\"" + key + "\"\\s*:\\s*\\[\\s*"
            "([-+0-9.eE]+)\\s*,\\s*"
            "([-+0-9.eE]+)\\s*,\\s*"
            "([-+0-9.eE]+)\\s*\\]"
        );

        std::smatch match;

        if (!std::regex_search(text, match, pattern)) {
            return defaultValue;
        }

        return {
            std::stof(match[1].str()),
            std::stof(match[2].str()),
            std::stof(match[3].str())
        };
    }

} // namespace

bool SceneObjectController::SaveEditorSceneToJson(
    const std::string& filePath
) const
{
    nlohmann::json sceneJson;

    sceneJson["version"] = 1;
    sceneJson["objects"] = nlohmann::json::array();

    for (size_t index = 3; index < objects_.size(); ++index) {
        const Object3d* object =
            GetObject(index);

        if (!object) {
            continue;
        }

        const Transform& transform =
            object->GetTransform();

        nlohmann::json objectJson;

        objectJson["name"] =
            GetObjectName(index);

        objectJson["model"] =
            GetObjectModelPath(index);

        objectJson["texture"] =
            GetObjectTexturePath(index);

        objectJson["visible"] =
            IsObjectVisible(index);

        objectJson["position"] = {
            transform.translate.x,
            transform.translate.y,
            transform.translate.z
        };

        objectJson["rotation"] = {
            transform.rotate.x,
            transform.rotate.y,
            transform.rotate.z
        };

        objectJson["scale"] = {
            transform.scale.x,
            transform.scale.y,
            transform.scale.z
        };

        sceneJson["objects"].push_back(objectJson);
    }

    std::ofstream file(filePath);

    if (!file.is_open()) {
        return false;
    }

    file << sceneJson.dump(4);

    return true;
}

bool SceneObjectController::LoadEditorSceneFromJson(
    const std::string& filePath
)
{
    std::ifstream file(filePath);

    if (!file.is_open()) {
        return false;
    }

    nlohmann::json sceneJson;

    try {
        file >> sceneJson;
    }
    catch (...) {
        return false;
    }

    if (!sceneJson.contains("objects") ||
        !sceneJson["objects"].is_array()) {
        return false;
    }

    ClearEditorObjects();

    for (const auto& objectJson : sceneJson["objects"]) {
        const std::string name =
            objectJson.value("name", "");

        const std::string modelPath =
            objectJson.value("model", "");

        const std::string texturePath =
            objectJson.value("texture", "");

        if (modelPath.empty()) {
            continue;
        }

        std::string directoryPath;
        std::string modelName;

        if (!SplitModelPath(
            modelPath,
            directoryPath,
            modelName
        )) {
            continue;
        }

        Model* model =
            ModelManager::Load(
                directoryPath,
                modelName
            );

        if (!model) {
            continue;
        }

        const size_t index =
            AddEditorObject(
                model,
                name.empty() ? modelName : name,
                modelPath
            );

        Object3d* object =
            GetObject(index);

        if (!object) {
            continue;
        }

        Transform& transform =
            object->GetTransform();

        if (objectJson.contains("position") &&
            objectJson["position"].is_array() &&
            objectJson["position"].size() >= 3) {
            transform.translate.x = objectJson["position"][0].get<float>();
            transform.translate.y = objectJson["position"][1].get<float>();
            transform.translate.z = objectJson["position"][2].get<float>();
        }

        if (objectJson.contains("rotation") &&
            objectJson["rotation"].is_array() &&
            objectJson["rotation"].size() >= 3) {
            transform.rotate.x = objectJson["rotation"][0].get<float>();
            transform.rotate.y = objectJson["rotation"][1].get<float>();
            transform.rotate.z = objectJson["rotation"][2].get<float>();
        }

        if (objectJson.contains("scale") &&
            objectJson["scale"].is_array() &&
            objectJson["scale"].size() >= 3) {
            transform.scale.x = objectJson["scale"][0].get<float>();
            transform.scale.y = objectJson["scale"][1].get<float>();
            transform.scale.z = objectJson["scale"][2].get<float>();
        }

        SetObjectVisible(
            index,
            objectJson.value("visible", true)
        );

        if (!texturePath.empty() &&
            object3dCommon_ &&
            object3dCommon_->GetTextureManager()) {

            const uint32_t textureHandle =
                object3dCommon_->GetTextureManager()->LoadTexture(texturePath);

            object->SetOverrideTexture(textureHandle);
            SetObjectTexturePath(index, texturePath);
        }
    }

    return true;
}
