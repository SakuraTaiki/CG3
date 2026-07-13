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

    bool hadLoadError = false;
    std::function<void(const nlohmann::json&, size_t)> loadObject;
    loadObject = [&](const nlohmann::json& objectJson, size_t parentIndex) {
        const std::string name =
            objectJson.value("name", "");

        std::string modelPath = objectJson.value("model", "");
        const std::string exportFileName = objectJson.value("file_name", "");

        if (modelPath.empty() && !exportFileName.empty()) {
            const std::string defaultPath =
                exportFileName.find('/') == std::string::npos &&
                exportFileName.find('\\') == std::string::npos
                ? "Resources/" + exportFileName
                : exportFileName;

            if (std::filesystem::exists(defaultPath)) {
                modelPath = defaultPath;
            } else {
                // Blender形式はfile_nameしか持たないため、Resourcesの
                // サブディレクトリも検索する。
                const std::filesystem::path targetName =
                    std::filesystem::path(exportFileName).filename();
                std::error_code error;

                for (std::filesystem::recursive_directory_iterator iterator(
                    "Resources",
                    std::filesystem::directory_options::skip_permission_denied,
                    error
                ), end; iterator != end && !error; iterator.increment(error)) {
                    if (iterator->is_regular_file(error) &&
                        iterator->path().filename() == targetName) {
                        modelPath = iterator->path().generic_string();
                        break;
                    }
                }
            }
        }

        const std::string texturePath =
            objectJson.value("texture", "");

        const std::string objectType = objectJson.value(
            "type",
            modelPath.empty() ? "EMPTY" : "MESH"
        );
        size_t index = kNoParent;
        std::string modelName;

        if (modelPath.empty()) {
            if (objectType == "MESH") {
                hadLoadError = true;
                return;
            }
            index = AddEditorEmpty(name.empty() ? "Empty" : name);
        } else {
            if (!std::filesystem::exists(modelPath)) {
                hadLoadError = true;
                return;
            }

            std::string directoryPath;
            if (!SplitModelPath(modelPath, directoryPath, modelName)) {
                hadLoadError = true;
                return;
            }

            Model* model = ModelManager::Load(directoryPath, modelName);
            if (!model) {
                hadLoadError = true;
                return;
            }

            index = AddEditorObject(
                model,
                name.empty() ? modelName : name,
                modelPath
            );
        }

        Object3d* object =
            GetObject(index);

        if (!object) {
            hadLoadError = true;
            return;
        }

        if (parentIndex != kNoParent && !SetObjectParent(index, parentIndex)) {
            hadLoadError = true;
            return;
        }

        SetObjectExportFileName(
            index,
            exportFileName.empty() ? modelName : exportFileName
        );

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

        if (objectJson.contains("transform") && objectJson["transform"].is_object()) {
            const auto& transformJson = objectJson["transform"];

            if (transformJson.contains("translation") &&
                transformJson["translation"].is_array() &&
                transformJson["translation"].size() >= 3) {
                transform.translate.x = transformJson["translation"][0].get<float>();
                transform.translate.y = transformJson["translation"][1].get<float>();
                transform.translate.z = transformJson["translation"][2].get<float>();
            }

            if (transformJson.contains("rotation") &&
                transformJson["rotation"].is_array() &&
                transformJson["rotation"].size() >= 3) {
                constexpr float kDegreesToRadians = 0.017453292519943295f;
                transform.rotate.x = transformJson["rotation"][0].get<float>() * kDegreesToRadians;
                transform.rotate.y = transformJson["rotation"][1].get<float>() * kDegreesToRadians;
                transform.rotate.z = transformJson["rotation"][2].get<float>() * kDegreesToRadians;
            }

            if (transformJson.contains("scaling") &&
                transformJson["scaling"].is_array() &&
                transformJson["scaling"].size() >= 3) {
                transform.scale.x = transformJson["scaling"][0].get<float>();
                transform.scale.y = transformJson["scaling"][1].get<float>();
                transform.scale.z = transformJson["scaling"][2].get<float>();
            }
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

        if (objectJson.contains("collider") && objectJson["collider"].is_object()) {
            const auto& colliderJson = objectJson["collider"];
            if (colliderJson.value("type", "") == "BOX") {
                AddBoxCollider(index);
                BoxCollider* collider = GetBoxCollider(index);
                if (colliderJson.contains("center") && colliderJson["center"].is_array() &&
                    colliderJson["center"].size() >= 3) {
                    collider->center = {
                        colliderJson["center"][0].get<float>(),
                        colliderJson["center"][1].get<float>(),
                        colliderJson["center"][2].get<float>()
                    };
                }
                if (colliderJson.contains("size") && colliderJson["size"].is_array() &&
                    colliderJson["size"].size() >= 3) {
                    collider->size = {
                        colliderJson["size"][0].get<float>(),
                        colliderJson["size"][1].get<float>(),
                        colliderJson["size"][2].get<float>()
                    };
                }
            }
        }

        if (objectJson.contains("children") && objectJson["children"].is_array()) {
            for (const auto& child : objectJson["children"]) {
                loadObject(child, index);
            }
        }
    };

    try {
        for (const auto& objectJson : sceneJson["objects"]) {
            loadObject(objectJson, kNoParent);
        }
    }
    catch (...) {
        ClearEditorObjects();
        return false;
    }

    return !hadLoadError;
}
