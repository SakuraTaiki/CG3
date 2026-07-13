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
