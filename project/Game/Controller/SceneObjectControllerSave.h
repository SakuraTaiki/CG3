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

