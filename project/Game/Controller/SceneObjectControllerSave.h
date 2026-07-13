bool SceneObjectController::SaveEditorSceneToJson(
    const std::string& filePath
) const
{
    std::function<nlohmann::json(size_t, int)> serializeObject;
    serializeObject = [&](size_t index, int level) -> nlohmann::json {
        const Object3d* object = GetObject(index);
        if (!object) {
            return nlohmann::json();
        }

        const Transform& transform = object->GetTransform();
        const bool hasModel = !GetObjectModelPath(index).empty();
        nlohmann::json objectJson;
        objectJson["name"] = GetObjectName(index);
        objectJson["type"] = hasModel ? "MESH" : "EMPTY";
        objectJson["level"] = level;
        objectJson["model"] = GetObjectModelPath(index);
        objectJson["file_name"] = GetObjectExportFileName(index);
        objectJson["texture"] = GetObjectTexturePath(index);
        objectJson["visible"] = IsObjectVisible(index);
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
        objectJson["transform"] = {
            { "translation", {
                transform.translate.x,
                transform.translate.y,
                transform.translate.z
            } },
            { "rotation", {
                transform.rotate.x * 57.29577951308232f,
                transform.rotate.y * 57.29577951308232f,
                transform.rotate.z * 57.29577951308232f
            } },
            { "scaling", {
                transform.scale.x,
                transform.scale.y,
                transform.scale.z
            } }
        };

        if (const BoxCollider* collider = GetBoxCollider(index)) {
            objectJson["collider"] = {
                { "type", "BOX" },
                { "center", {
                    collider->center.x,
                    collider->center.y,
                    collider->center.z
                } },
                { "size", {
                    collider->size.x,
                    collider->size.y,
                    collider->size.z
                } }
            };
        }

        for (size_t child = 3; child < objects_.size(); ++child) {
            if (GetObjectParent(child) == index) {
                if (!objectJson.contains("children")) {
                    objectJson["children"] = nlohmann::json::array();
                }
                objectJson["children"].push_back(
                    serializeObject(child, level + 1)
                );
            }
        }
        return objectJson;
    };

    nlohmann::json sceneJson;
    sceneJson["version"] = 3;
    sceneJson["name"] = "EditorScene";
    sceneJson["objects"] = nlohmann::json::array();

    for (size_t index = 3; index < objects_.size(); ++index) {
        const size_t parent = GetObjectParent(index);
        if (parent == kNoParent || parent < 3) {
            sceneJson["objects"].push_back(serializeObject(index, 0));
        }
    }

    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    file << sceneJson.dump(4) << '\n';
    return true;
}

bool SceneObjectController::ExportLevelSceneToJson(
    const std::string& filePath
) const
{
    std::function<nlohmann::json(size_t, int)> serializeObject;
    serializeObject = [&](size_t index, int level) -> nlohmann::json {
        const Object3d* object = GetObject(index);
        if (!object) {
            return nlohmann::json();
        }

        const Transform& transform = object->GetTransform();
        const bool hasModel = !GetObjectModelPath(index).empty();
        nlohmann::json objectJson;
        objectJson["type"] = hasModel ? "MESH" : "EMPTY";
        objectJson["name"] = GetObjectName(index);
        objectJson["level"] = level;
        objectJson["transform"] = {
            { "translation", {
                transform.translate.x,
                transform.translate.y,
                transform.translate.z
            } },
            { "rotation", {
                transform.rotate.x * 57.29577951308232f,
                transform.rotate.y * 57.29577951308232f,
                transform.rotate.z * 57.29577951308232f
            } },
            { "scaling", {
                transform.scale.x,
                transform.scale.y,
                transform.scale.z
            } }
        };

        if (hasModel) {
            objectJson["file_name"] = GetObjectExportFileName(index);
            objectJson["model"] = GetObjectModelPath(index);
        }

        if (const BoxCollider* collider = GetBoxCollider(index)) {
            objectJson["collider"] = {
                { "type", "BOX" },
                { "center", {
                    collider->center.x,
                    collider->center.y,
                    collider->center.z
                } },
                { "size", {
                    collider->size.x,
                    collider->size.y,
                    collider->size.z
                } }
            };
        }

        for (size_t child = 3; child < objects_.size(); ++child) {
            if (GetObjectParent(child) == index) {
                if (!objectJson.contains("children")) {
                    objectJson["children"] = nlohmann::json::array();
                }
                objectJson["children"].push_back(
                    serializeObject(child, level + 1)
                );
            }
        }
        return objectJson;
    };

    nlohmann::json sceneJson;
    sceneJson["name"] = "EditorScene";
    sceneJson["objects"] = nlohmann::json::array();

    for (size_t index = 3; index < objects_.size(); ++index) {
        const size_t parent = GetObjectParent(index);
        if (parent == kNoParent || parent < 3) {
            sceneJson["objects"].push_back(serializeObject(index, 0));
        }
    }

    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    file << sceneJson.dump(4) << '\n';
    return true;
}
