#pragma once

#include "SceneDebugPanelConsole.h"

#include "EngineContext.h"
#include "SceneObjectController.h"
#include "Object3dCommon.h"
#include "ModelManager.h"
#include "TextureManager.h"

#include <cstddef>
#include <cstdint>
#include <string>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

namespace SceneDebugPanelDetail {
    inline std::string& GetSelectedAssetPath() {
        static std::string selectedAssetPath = "";
        return selectedAssetPath;
    }

    inline bool EndsWith(const std::string& text, const char* suffix) {
        const std::string suffixText = suffix;

        if (text.size() < suffixText.size()) {
            return false;
        }

        return text.compare(
            text.size() - suffixText.size(),
            suffixText.size(),
            suffixText
        ) == 0;
    }

    inline const char* GetAssetTypeFromPath(const std::string& path) {
        if (EndsWith(path, ".obj")) {
            return "Model";
        }

        if (EndsWith(path, ".gltf")) {
            return "glTF Model";
        }

        if (EndsWith(path, ".fbx")) {
            return "FBX Model";
        }

        if (EndsWith(path, ".png")) {
            return "Texture";
        }

        if (EndsWith(path, ".hlsl") || EndsWith(path, ".hlsli")) {
            return "Shader";
        }

        if (EndsWith(path, ".txt")) {
            return "Text";
        }

        if (EndsWith(path, ".mtl")) {
            return "Material";
        }

        return "Unknown";
    }

    inline void SelectAsset(const std::string& path) {
        GetSelectedAssetPath() = path;
        AddEditorLog(EditorLogType::Info, "Selected asset: " + path);
    }


    inline size_t& GetModelApplyTargetIndex() {
        static size_t targetIndex = 0;

        return targetIndex;
    }

    inline std::string GetModelApplyTargetName(const SceneObjectController& sceneObjects) {
        const size_t targetIndex =
            GetModelApplyTargetIndex();

        if (targetIndex >= sceneObjects.GetObjectCount()) {
            return "Unknown";
        }

        return sceneObjects.GetObjectName(targetIndex);
    }


    inline void DrawModelApplyTargetCombo(SceneObjectController& sceneObjects) {
#ifdef USE_IMGUI
        if (sceneObjects.GetObjectCount() == 0) {
            ImGui::TextDisabled("Target : None");
            return;
        }

        size_t& targetIndex =
            GetModelApplyTargetIndex();

        int currentTarget =
            static_cast<int>(targetIndex);

        if (currentTarget < 0 ||
            currentTarget >= static_cast<int>(sceneObjects.GetObjectCount())) {
            currentTarget = 0;
            targetIndex = 0;
        }

        const std::string preview =
            sceneObjects.GetObjectName(static_cast<size_t>(currentTarget));

        if (ImGui::BeginCombo("Target", preview.c_str())) {
            for (size_t index = 0; index < sceneObjects.GetObjectCount(); ++index) {
                const bool selected =
                    index == targetIndex;

                if (ImGui::Selectable(
                    sceneObjects.GetObjectName(index).c_str(),
                    selected
                )) {
                    targetIndex = index;
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        if (targetIndex != static_cast<size_t>(currentTarget)) {

            AddEditorLog(
                EditorLogType::Info,
                "Apply target changed: " + GetModelApplyTargetName(sceneObjects)
            );
        }
#endif
    }


    inline Object3d* GetModelApplyTargetObject(SceneObjectController& sceneObjects) {
        return sceneObjects.GetObject(
            GetModelApplyTargetIndex()
        );
    }

    inline bool SplitResourceModelPath(
        const std::string& path,
        std::string& directoryPath,
        std::string& modelName
    ) {
        if (!EndsWith(path, ".obj")) {
            return false;
        }

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

    inline bool ApplySelectedObjModelToObject(
        Object3d* object,
        const SceneObjectController& sceneObjects
    ) {
#ifdef USE_IMGUI
        if (!object) {
            AddEditorLog(EditorLogType::Error, "Apply failed: target object is null.");
            return false;
        }

        const std::string& assetPath =
            GetSelectedAssetPath();

        if (assetPath.empty()) {
            AddEditorLog(EditorLogType::Warning, "Apply failed: no asset selected.");
            return false;
        }

        if (!EndsWith(assetPath, ".obj")) {
            AddEditorLog(EditorLogType::Warning, "Only .obj model assets can be applied now.");
            return false;
        }

        std::string directoryPath;
        std::string modelName;

        if (!SplitResourceModelPath(
            assetPath,
            directoryPath,
            modelName
        )) {
            AddEditorLog(EditorLogType::Error, "Apply failed: invalid model path.");
            return false;
        }

        Model* model =
            ModelManager::Load(
                directoryPath,
                modelName
            );

        if (!model) {
            AddEditorLog(EditorLogType::Error, "Apply failed: model load returned null.");
            return false;
        }

        object->SetModel(model);

        AddEditorLog(
            EditorLogType::Info,
            "Applied model: " + assetPath + " -> " + GetModelApplyTargetName(sceneObjects)
        );

        return true;
#else
        return false;
#endif
    }

    inline size_t CreateObjectFromSelectedObjAsset(SceneObjectController& sceneObjects) {
#ifdef USE_IMGUI
        const std::string& assetPath =
            GetSelectedAssetPath();

        if (assetPath.empty()) {
            AddEditorLog(EditorLogType::Warning, "Create object failed: no asset selected.");
            return static_cast<size_t>(-1);
        }

        if (!EndsWith(assetPath, ".obj")) {
            AddEditorLog(EditorLogType::Warning, "Create object failed: only .obj assets can create objects now.");
            return static_cast<size_t>(-1);
        }

        std::string directoryPath;
        std::string modelName;

        if (!SplitResourceModelPath(
            assetPath,
            directoryPath,
            modelName
        )) {
            AddEditorLog(EditorLogType::Error, "Create object failed: invalid model path.");
            return static_cast<size_t>(-1);
        }

        Model* model =
            ModelManager::Load(
                directoryPath,
                modelName
            );

        if (!model) {
            AddEditorLog(EditorLogType::Error, "Create object failed: model load returned null.");
            return static_cast<size_t>(-1);
        }

        const std::string objectName =
            "New " + modelName;

        const size_t objectIndex =
            sceneObjects.AddEditorObject(
                model,
                objectName,
                assetPath
            );

        if (objectIndex == static_cast<size_t>(-1)) {
            AddEditorLog(EditorLogType::Error, "Create object failed: SceneObjectController rejected object.");
            return objectIndex;
        }

        GetModelApplyTargetIndex() =
            objectIndex;

        AddEditorLog(
            EditorLogType::Info,
            "Created object: " + objectName
        );

        return objectIndex;
#else
        return static_cast<size_t>(-1);
#endif
    }


    inline bool ApplySelectedPngTextureToObject(
        Object3d* object,
        TextureManager* textureManager,
        SceneObjectController& sceneObjects
    ) {
#ifdef USE_IMGUI
        if (!object) {
            AddEditorLog(EditorLogType::Error, "Texture apply failed: target object is null.");
            return false;
        }

        if (!textureManager) {
            AddEditorLog(EditorLogType::Error, "Texture apply failed: TextureManager is null.");
            return false;
        }

        const std::string& assetPath =
            GetSelectedAssetPath();

        if (assetPath.empty()) {
            AddEditorLog(EditorLogType::Warning, "Texture apply failed: no asset selected.");
            return false;
        }

        if (!EndsWith(assetPath, ".png")) {
            AddEditorLog(EditorLogType::Warning, "Only .png texture assets can be applied now.");
            return false;
        }

        uint32_t textureHandle =
            textureManager->LoadTexture(assetPath);

        object->SetOverrideTexture(textureHandle);

        sceneObjects.SetObjectTexturePath(
            GetModelApplyTargetIndex(),
            assetPath
        );

        AddEditorLog(
            EditorLogType::Info,
            "Applied texture: " + assetPath +
            " -> " + GetModelApplyTargetName(sceneObjects) +
            " handle=" + std::to_string(textureHandle)
        );

        return true;
#else
        return false;
#endif
    }


    inline bool DrawAssetItem(const char* label, const char* path) {
#ifdef USE_IMGUI
        const bool selected =
            GetSelectedAssetPath() == path;

        if (ImGui::Selectable(label, selected)) {
            SelectAsset(path);
            return true;
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(path);
            ImGui::EndTooltip();
        }
#endif

        return false;
    }

    inline void DrawTransformInspector(Object3d* object) {
#ifdef USE_IMGUI
        if (!object) {
            ImGui::TextDisabled("Object is null.");
            return;
        }

        Transform& transform = object->GetTransform();

        if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            return;
        }

        ImGui::DragFloat3(
            "Position",
            &transform.translate.x,
            0.05f
        );

        ImGui::DragFloat3(
            "Rotation",
            &transform.rotate.x,
            0.01f
        );

        ImGui::DragFloat3(
            "Scale",
            &transform.scale.x,
            0.01f,
            0.001f,
            100.0f
        );
#endif
    }




    inline void DrawMaterialInspector(Object3d* object) {
#ifdef USE_IMGUI
        if (!object) {
            ImGui::TextDisabled("Object is null.");
            return;
        }

        Material* material = object->GetMaterial();

        if (!material) {
            ImGui::TextDisabled("Material is null.");
            return;
        }

        if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool enableLighting = material->enableLighting != 0;

            if (ImGui::Checkbox("Enable Lighting", &enableLighting)) {
                material->enableLighting = enableLighting ? 1 : 0;
            }

            ImGui::ColorEdit4(
                "Color",
                &material->color.x
            );

            ImGui::SliderFloat(
                "Environment",
                &material->environmentCoefficient,
                0.0f,
                2.0f,
                "%.2f"
            );

            ImGui::SeparatorText("Override Texture");

            if (object->HasOverrideTexture()) {
                ImGui::Text(
                    "Override Handle : %u",
                    object->GetOverrideTextureHandle()
                );
            } else {
                ImGui::TextDisabled("Override Texture : None");
            }
        }
#endif
    }


    inline bool DrawAssetInspector(
        EngineContext* context,
        SceneObjectController& sceneObjects,
        size_t& outCreatedIndex
    ) {
#ifdef USE_IMGUI
        outCreatedIndex = static_cast<size_t>(-1);

        const std::string& path =
            GetSelectedAssetPath();

        ImGui::Text("Asset Project Asset");
        ImGui::Separator();

        if (path.empty()) {
            ImGui::TextDisabled("No asset selected.");
            return false;
        }

        ImGui::SeparatorText("Asset Info");

        ImGui::Text("Path");
        ImGui::TextWrapped("%s", path.c_str());

        ImGui::Spacing();

        ImGui::Text("Type");
        ImGui::TextDisabled("%s", GetAssetTypeFromPath(path));

        ImGui::Spacing();

        if (ImGui::Button("Copy Path")) {
            ImGui::SetClipboardText(path.c_str());
            AddEditorLog(EditorLogType::Info, "Copied asset path to clipboard.");
        }

        ImGui::SameLine();

        if (ImGui::Button("Ping")) {
            AddEditorLog(EditorLogType::Info, "Ping asset: " + path);
        }

        ImGui::SeparatorText("Apply Target");

        DrawModelApplyTargetCombo(sceneObjects);

        Object3d* targetObject =
            GetModelApplyTargetObject(sceneObjects);

        TextureManager* textureManager = nullptr;

        if (context && context->GetObject3dCommon()) {
            textureManager =
                context->GetObject3dCommon()->GetTextureManager();
        }

        const bool canApplyModel =
            EndsWith(path, ".obj");

        const bool canApplyTexture =
            EndsWith(path, ".png");

        ImGui::SeparatorText("Model");

        ImGui::BeginDisabled(!canApplyModel);

        if (ImGui::Button("Apply Model To Target")) {
            ApplySelectedObjModelToObject(
                targetObject,
                sceneObjects
            );
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!canApplyModel);

        bool createdObjectFromAsset = false;

        if (ImGui::Button("Create Object From Asset")) {
            const size_t createdIndex =
                CreateObjectFromSelectedObjAsset(sceneObjects);

            if (createdIndex != static_cast<size_t>(-1)) {
                outCreatedIndex = createdIndex;
                createdObjectFromAsset = true;
            }
        }

        ImGui::EndDisabled();

        if (createdObjectFromAsset) {
            return true;
        }

        ImGui::Spacing();
        ImGui::SeparatorText("Texture");

        ImGui::TextDisabled("Texture Target");

        if (targetObject) {
            ImGui::Text(
                "Current Target : %s",
                GetModelApplyTargetName(sceneObjects).c_str()
            );
        } else {
            ImGui::TextDisabled("Current Target : None");
        }

        ImGui::BeginDisabled(!canApplyTexture || !targetObject || !textureManager);

        if (ImGui::Button("Apply Texture To Target")) {
            ApplySelectedPngTextureToObject(
                targetObject,
                textureManager,
                sceneObjects
            );
        }

        ImGui::EndDisabled();


        ImGui::SameLine();

        ImGui::BeginDisabled(!targetObject || !targetObject->HasOverrideTexture());

        if (ImGui::Button("Clear Texture")) {
            targetObject->ClearOverrideTexture();
            sceneObjects.SetObjectTexturePath(
                GetModelApplyTargetIndex(),
                ""
            );
            AddEditorLog(
                EditorLogType::Info,
                "Cleared override texture: " + GetModelApplyTargetName(sceneObjects)
            );
        }

        ImGui::EndDisabled();


        if (!canApplyModel && !canApplyTexture) {
            ImGui::TextDisabled("Only .obj models and .png textures can be applied now.");
        }

        ImGui::SeparatorText("Preview");

        if (EndsWith(path, ".png")) {
            ImGui::TextDisabled("Texture preview will be added in a later step.");
        } else if (EndsWith(path, ".obj")) {
            ImGui::TextDisabled("OBJ model asset.");
        } else if (EndsWith(path, ".gltf") || EndsWith(path, ".fbx")) {
            ImGui::TextDisabled("glTF / FBX apply will be added later.");
        } else if (EndsWith(path, ".hlsl") || EndsWith(path, ".hlsli")) {
            ImGui::TextDisabled("Shader source preview will be added later.");
        } else {
            ImGui::TextDisabled("No preview available.");
        }
        return false;
#else
        return false;
#endif
    }
} // namespace SceneDebugPanelDetail
