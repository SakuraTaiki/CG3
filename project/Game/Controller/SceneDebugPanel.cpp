#include "SceneDebugPanel.h"

#include "EngineContext.h"
#include "HitEffectController.h"
#include "AnimationDebugController.h"
#include "SoundController.h"
#include "CameraDebugController.h"
#include "EnvironmentController.h"
#include "SceneObjectController.h"
#include "Object3dCommon.h"



#include "Ring.h"
#include "Cylinder.h"
#include "Primitive.h"
#include "ParticleManager.h"
#include "DirectXCommon.h"
#include "ImGuiManager.h"
#include "camera.h"
#include "GPUParticleManager.h"
#include "ModelManager.h"
#include "TextureManager.h"


#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/ImGuizmo.h"
#endif


namespace {

    enum class EditorLogType {
        Info,
        Warning,
        Error
    };

    struct EditorConsoleLog {
        EditorLogType type;
        std::string message;
    };

    std::vector<EditorConsoleLog>& GetEditorLogs() {
        static std::vector<EditorConsoleLog> logs;
        return logs;
    }

    void AddEditorLog(EditorLogType type, const std::string& message) {
        GetEditorLogs().push_back({ type, message });
    }

    void InitializeEditorConsoleOnce() {
        static bool initialized = false;

        if (initialized) {
            return;
        }

        initialized = true;

        AddEditorLog(EditorLogType::Info, "Editor console initialized.");
        AddEditorLog(EditorLogType::Info, "Hierarchy / Inspector / Project panels are active.");
        AddEditorLog(EditorLogType::Warning, "Scene View is not separated yet. Rendering uses main back buffer.");
    }

    const char* GetEditorLogPrefix(EditorLogType type) {
        switch (type) {
        case EditorLogType::Info:
            return "[Info]";

        case EditorLogType::Warning:
            return "[Warning]";

        case EditorLogType::Error:
            return "[Error]";

        default:
            return "[Log]";
        }
    }

    ImVec4 GetEditorLogColor(EditorLogType type) {
        switch (type) {
        case EditorLogType::Info:
            return ImVec4(0.55f, 0.85f, 0.55f, 1.0f);

        case EditorLogType::Warning:
            return ImVec4(1.0f, 0.75f, 0.25f, 1.0f);

        case EditorLogType::Error:
            return ImVec4(1.0f, 0.35f, 0.35f, 1.0f);

        default:
            return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    std::string& GetSelectedAssetPath() {
        static std::string selectedAssetPath = "";
        return selectedAssetPath;
    }

    bool EndsWith(const std::string& text, const char* suffix) {
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

    const char* GetAssetTypeFromPath(const std::string& path) {
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

    void SelectAsset(const std::string& path) {
        GetSelectedAssetPath() = path;
        AddEditorLog(EditorLogType::Info, "Selected asset: " + path);
    }


    size_t& GetModelApplyTargetIndex() {
        static size_t targetIndex = 0;

        return targetIndex;
    }

    std::string GetModelApplyTargetName(const SceneObjectController& sceneObjects) {
        const size_t targetIndex =
            GetModelApplyTargetIndex();

        if (targetIndex >= sceneObjects.GetObjectCount()) {
            return "Unknown";
        }

        return sceneObjects.GetObjectName(targetIndex);
    }


    void DrawModelApplyTargetCombo(SceneObjectController& sceneObjects) {
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


    Object3d* GetModelApplyTargetObject(SceneObjectController& sceneObjects) {
        return sceneObjects.GetObject(
            GetModelApplyTargetIndex()
        );
    }

    bool SplitResourceModelPath(
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

    bool ApplySelectedObjModelToObject(
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

    size_t CreateObjectFromSelectedObjAsset(SceneObjectController& sceneObjects) {
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


    bool ApplySelectedPngTextureToObject(
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


    bool DrawAssetItem(const char* label, const char* path) {
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

    void DrawTransformInspector(Object3d* object) {
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




    void DrawMaterialInspector(Object3d* object) {
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


    bool DrawAssetInspector(
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
}

void SceneDebugPanel::Draw(
    EngineContext* context,
    GameSceneDrawMode& drawMode,
    HitEffectController& hitEffect,
    AnimationDebugController& animationDebug,
    SoundController& soundController,
    CameraDebugController& cameraDebug,
    EnvironmentController& environment,
    SceneObjectController& sceneObjects,
    Ring* ring,
    Cylinder* cylinder,
    Primitive* primitive
) {
#ifdef USE_IMGUI
    if (!context) {
        return;
    }

    context->GetImGuiManager()->Begin();

    cameraDebug.Update(
        context->GetCamera()
    );

    DrawMainMenuBar(
        drawMode,
        hitEffect,
        animationDebug,
        sceneObjects
    );


    DrawHierarchyWindow(
        sceneObjects
    );

    DrawInspectorWindow(
        context,
        environment,
        sceneObjects,
        animationDebug
    );

    DrawProjectWindow();

    DrawGameViewWindow(context, sceneObjects);

    DrawConsoleWindow();

    DrawDebugToolsWindow(
        context,
        drawMode,
        hitEffect,
        animationDebug,
        soundController,
        environment,
        sceneObjects,
        ring,
        cylinder,
        primitive
    );

    context->GetImGuiManager()->End();
#endif
}

void SceneDebugPanel::DrawMainMenuBar(
    GameSceneDrawMode& drawMode,
    HitEffectController& hitEffect,
    AnimationDebugController& animationDebug,
    SceneObjectController& sceneObjects
) {
#ifdef USE_IMGUI
    //==================================
    //Docking対応版Imgui
    //=================================

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New Scene", nullptr, false, false);
            ImGui::Separator();

            if (ImGui::MenuItem("Save Scene Json", "Ctrl+S")) {
                if (sceneObjects.SaveEditorSceneToJson("Resources/EditorScene.json")) {
                    AddEditorLog(
                        EditorLogType::Info,
                        "Saved scene json: Resources/EditorScene.json"
                    );
                } else {
                    AddEditorLog(
                        EditorLogType::Error,
                        "Save scene json failed."
                    );
                }
            }

            if (ImGui::MenuItem("Load Scene Json", "Ctrl+O")) {
                if (sceneObjects.LoadEditorSceneFromJson("Resources/EditorScene.json")) {
                    selected_ = EditorSelection::None;
                    selectedObjectIndex_ = 0;
                    GetModelApplyTargetIndex() = 0;

                    AddEditorLog(
                        EditorLogType::Info,
                        "Loaded scene json: Resources/EditorScene.json"
                    );
                } else {
                    AddEditorLog(
                        EditorLogType::Error,
                        "Load scene json failed."
                    );
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
            ImGui::MenuItem("Redo", "Ctrl+Y", false, false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GameObject")) {
            if (ImGui::MenuItem("Terrain")) {
                selected_ = EditorSelection::Terrain;
            }

            if (ImGui::MenuItem("Axis +")) {
                selected_ = EditorSelection::AxisPositive;
            }

            if (ImGui::MenuItem("Axis -")) {
                selected_ = EditorSelection::AxisNegative;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Hierarchy");
            ImGui::MenuItem("Inspector");
            ImGui::MenuItem("Project");
            ImGui::MenuItem("Console");
            ImGui::MenuItem("Debug Tools");
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::Button("Play")) {
            drawMode = GameSceneDrawMode::NormalObj;
            AddEditorLog(EditorLogType::Info, "Play Button pressed.");
        }

        ImGui::SameLine();

        if (ImGui::Button("Emit")) {
            hitEffect.Emit({ 0.0f, 3.0f, 0.0f });
            AddEditorLog(EditorLogType::Info, "Effect emitted from menu bar.");
        }

        ImGui::SameLine();
        ImGui::TextDisabled(
            "Joints:%d  Anim:%.2f",
            animationDebug.GetJointCount(),
            animationDebug.GetAnimationTime()
        );

        ImGui::EndMainMenuBar();
    }

#endif
}

void SceneDebugPanel::DrawHierarchyWindow(
    SceneObjectController& sceneObjects
) {
#ifdef USE_IMGUI
    ImGui::Begin("Hierarchy");

    ImGui::TextDisabled("Sample Scene");
    ImGui::Separator();

    ImGuiTreeNodeFlags rootFlags =
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (ImGui::TreeNodeEx("Scene", rootFlags)) {
        if (ImGui::Selectable(
            "Terrain",
            selected_ == EditorSelection::Terrain
        )) {
            selected_ = EditorSelection::Terrain;
            selectedObjectIndex_ = 0;
            GetModelApplyTargetIndex() = 0;
        }

        if (ImGui::Selectable(
            "Axis +X",
            selected_ == EditorSelection::AxisPositive
        )) {
            selected_ = EditorSelection::AxisPositive;
            selectedObjectIndex_ = 1;
            GetModelApplyTargetIndex() = 1;
        }

        if (ImGui::Selectable(
            "Axis -X",
            selected_ == EditorSelection::AxisNegative
        )) {
            selected_ = EditorSelection::AxisNegative;
            selectedObjectIndex_ = 2;
            GetModelApplyTargetIndex() = 2;
        }

        ImGui::Separator();

        if (sceneObjects.GetObjectCount() > 3) {
            ImGui::TextDisabled("Created Objects");

            for (size_t index = 3; index < sceneObjects.GetObjectCount(); ++index) {
                if (ImGui::Selectable(
                    sceneObjects.GetObjectName(index).c_str(),
                    selected_ == EditorSelection::DynamicObject &&
                    selectedObjectIndex_ == index
                )) {
                    selected_ = EditorSelection::DynamicObject;
                    selectedObjectIndex_ = index;
                    GetModelApplyTargetIndex() = index;
                }
            }

            ImGui::Separator();
        }

        if (ImGui::Selectable(
            "Main Camera",
            selected_ == EditorSelection::Camera
        )) {
            selected_ = EditorSelection::Camera;
        }

        if (ImGui::Selectable(
            "Environment",
            selected_ == EditorSelection::Environment
        )) {
            selected_ = EditorSelection::Environment;
        }

        if (ImGui::Selectable(
            "Effects",
            selected_ == EditorSelection::Effects
        )) {
            selected_ = EditorSelection::Effects;
        }

        ImGui::TreePop();
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Visibility");

    ImGui::Checkbox(
        "Show Terrain",
        &sceneObjects.ShowTerrain()
    );

    ImGui::Checkbox(
        "Show Axis",
        &sceneObjects.ShowAxis()
    );

    for (size_t index = 3; index < sceneObjects.GetObjectCount(); ++index) {
        bool visible =
            sceneObjects.IsObjectVisible(index);

        const std::string label =
            "Show " + sceneObjects.GetObjectName(index);

        if (ImGui::Checkbox(label.c_str(), &visible)) {
            sceneObjects.SetObjectVisible(index, visible);
        }
    }

    ImGui::End();
#endif
}


void SceneDebugPanel::DrawInspectorWindow(
    EngineContext* context,
    EnvironmentController& environment,
    SceneObjectController& sceneObjects,
    AnimationDebugController& animationDebug
) {
#ifdef USE_IMGUI
    ImGui::Begin("Inspector");

    if (selected_ == EditorSelection::None) {
        ImGui::TextDisabled("No object selected.");
        ImGui::End();
        return;
    }

    auto DrawHeader = [](const char* icon, const char* name) {
        ImGui::Text("%s %s", icon, name);
        ImGui::Separator();
        };

    switch (selected_) {
    case EditorSelection::Terrain:

    {
        DrawHeader("Object", "Terrain");

        ImGui::Checkbox(
            "Active",
            &sceneObjects.ShowTerrain()
        );

        Object3d* object =
            sceneObjects.GetEditorObject(
                SceneObjectController::EditorObjectType::Terrain
            );

        DrawTransformInspector(object);
        DrawMaterialInspector(object);

        if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("Resources/terrain/terrain.obj");
        }

        break;
    }

    case EditorSelection::AxisPositive:

    {
        DrawHeader("Axis", "Axis +X");

        ImGui::Checkbox(
            "Active",
            &sceneObjects.ShowAxis()
        );


        Object3d* object =
            sceneObjects.GetEditorObject(
                SceneObjectController::EditorObjectType::AxisPositive
            );

        DrawTransformInspector(object);
        DrawMaterialInspector(object);

        if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("axis.obj");
        }


        break;
    }

    case EditorSelection::AxisNegative:

    {
        DrawHeader("Axis", "Axis -X");

        ImGui::Checkbox(
            "Active",
            &sceneObjects.ShowAxis()
        );

        Object3d* object =
            sceneObjects.GetEditorObject(
                SceneObjectController::EditorObjectType::AxisNegative
            );

        DrawTransformInspector(object);
        DrawMaterialInspector(object);

        if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("axis.obj");
        }

        break;
    }

    case EditorSelection::Camera:
        DrawHeader("▣", "Main Camera");

        if (context && context->GetCamera()) {
            ImGui::TextDisabled("Camera debug controller is active.");
            ImGui::TextDisabled("Right drag : Orbit");
            ImGui::TextDisabled("Wheel      : Zoom");
        }
        break;

    case EditorSelection::Environment:
        DrawHeader("☀", "Environment");

        DrawEnvironmentTab(
            context ? context->GetObject3dCommon() : nullptr,
            environment,
            sceneObjects,
            animationDebug
        );
        break;

    case EditorSelection::Effects:
        DrawHeader("✦", "Effects");
        ImGui::TextDisabled("Open Debug Tools > Effect for detailed controls.");
        break;

    case EditorSelection::PostEffect:
        DrawHeader("▣", "Post Effect");
        DrawPostEffectTab(context);
        break;

    case EditorSelection::Sound:
        DrawHeader("♪", "Sound");
        ImGui::TextDisabled("Open Debug Tools > Sound for detailed controls.");
        break;

    case EditorSelection::DynamicObject:
    {
        const std::string& objectName =
            sceneObjects.GetObjectName(selectedObjectIndex_);

        DrawHeader("Object", objectName.empty() ? "Created Object" : objectName.c_str());

        bool visible =
            sceneObjects.IsObjectVisible(selectedObjectIndex_);

        if (ImGui::Checkbox("Active", &visible)) {
            sceneObjects.SetObjectVisible(selectedObjectIndex_, visible);
        }

        Object3d* object =
            sceneObjects.GetObject(selectedObjectIndex_);

        DrawTransformInspector(object);
        DrawMaterialInspector(object);

        if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("%s", objectName.c_str());
        }

        break;
    }

    default:
        ImGui::TextDisabled("No inspector available.");
        break;

    case EditorSelection::Asset:
    {
        size_t createdIndex = static_cast<size_t>(-1);

        if (DrawAssetInspector(
            context,
            sceneObjects,
            createdIndex
        )) {
            selected_ = EditorSelection::DynamicObject;
            selectedObjectIndex_ = createdIndex;
        }

        break;
    }
    }

    ImGui::End();
#endif
}


void SceneDebugPanel::DrawProjectWindow() {
#ifdef USE_IMGUI
    ImGui::Begin("Project");

    auto DrawProjectAsset = [this](const char* label, const char* path) {
        if (DrawAssetItem(label, path)) {
            selected_ = EditorSelection::Asset;
        }
        };

    ImGui::TextDisabled("Assets");
    ImGui::Separator();

    if (ImGui::BeginTable(
        "ProjectBrowserTable",
        2,
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_RowBg
    )) {
        ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);

        ImGui::TextDisabled("Favorites");
        ImGui::Separator();

        ImGui::Selectable("Resources", true);
        ImGui::Selectable("Models");
        ImGui::Selectable("Textures");
        ImGui::Selectable("Shaders");
        ImGui::Selectable("Settings");

        ImGui::TableSetColumnIndex(1);

        if (ImGui::TreeNodeEx(
            "Models",
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth
        )) {
            DrawProjectAsset("axis.obj", "Resources/axis.obj");
            DrawProjectAsset("plane.obj", "Resources/plane.obj");
            DrawProjectAsset("terrain.obj", "Resources/terrain/terrain.obj");
            DrawProjectAsset("bunny.obj", "Resources/bunny/bunny.obj");
            DrawProjectAsset("fence.obj", "Resources/fence/fence.obj");
            DrawProjectAsset("AnimatedCube.gltf", "Resources/AnimatedCube/AnimatedCube.gltf");
            DrawProjectAsset("human/walk.gltf", "Resources/human/walk.gltf");
            DrawProjectAsset("human/walk.fbx", "Resources/human/walk.fbx");

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx(
            "Textures",
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth
        )) {
            DrawProjectAsset("monsterBall.png", "Resources/monsterBall.png");
            DrawProjectAsset("noise0.png", "Resources/noise0.png");
            DrawProjectAsset("uvChecker.png", "Resources/uvChecker.png");
            DrawProjectAsset("white.png", "Resources/white.png");
            DrawProjectAsset("fence.png", "Resources/fence/fence.png");

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx(
            "Shaders",
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth
        )) {
            DrawProjectAsset("Object3d.VS.hlsl", "Resources/shaders/hlsl/Object3d.VS.hlsl");
            DrawProjectAsset("Object3d.PS.hlsl", "Resources/shaders/hlsl/Object3d.PS.hlsl");
            DrawProjectAsset("Particle.VS.hlsl", "Resources/shaders/hlsl/Particle.VS.hlsl");
            DrawProjectAsset("Particle.PS.hlsl", "Resources/shaders/hlsl/Particle.PS.hlsl");
            DrawProjectAsset("CopyImage.PS.hlsl", "Resources/shaders/hlsl/CopyImage.PS.hlsl");
            DrawProjectAsset("GaussianFilter.PS.hlsl", "Resources/shaders/hlsl/GaussianFilter.PS.hlsl");

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx(
            "Settings",
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth
        )) {
            DrawProjectAsset("Fire.txt", "Resources/Settings/HitEffects/Fire.txt");
            DrawProjectAsset("cherryBlossoms.txt", "Resources/Settings/HitEffects/cherryBlossoms.txt");

            ImGui::TreePop();
        }

        ImGui::EndTable();
    }

    ImGui::End();
#endif
}


void SceneDebugPanel::DrawConsoleWindow() {
#ifdef USE_IMGUI
    InitializeEditorConsoleOnce();

    static bool showInfo = true;
    static bool showWarning = true;
    static bool showError = true;
    static bool autoScroll = true;

    ImGui::Begin("Console");

    if (ImGui::Button("Clear")) {
        GetEditorLogs().clear();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Info", &showInfo);

    ImGui::SameLine();
    ImGui::Checkbox("Warning", &showWarning);

    ImGui::SameLine();
    ImGui::Checkbox("Error", &showError);

    ImGui::SameLine();
    ImGui::Checkbox("Auto Scroll", &autoScroll);

    ImGui::Separator();

    ImGui::BeginChild(
        "ConsoleLogArea",
        ImVec2(0.0f, 0.0f),
        false,
        ImGuiWindowFlags_HorizontalScrollbar
    );

    for (const EditorConsoleLog& log : GetEditorLogs()) {
        if (log.type == EditorLogType::Info && !showInfo) {
            continue;
        }

        if (log.type == EditorLogType::Warning && !showWarning) {
            continue;
        }

        if (log.type == EditorLogType::Error && !showError) {
            continue;
        }

        ImGui::TextColored(
            GetEditorLogColor(log.type),
            "%s %s",
            GetEditorLogPrefix(log.type),
            log.message.c_str()
        );
    }

    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();

    ImGui::End();
#endif
}


void SceneDebugPanel::DrawDebugToolsWindow(
    EngineContext* context,
    GameSceneDrawMode& drawMode,
    HitEffectController& hitEffect,
    AnimationDebugController& animationDebug,
    SoundController& soundController,
    EnvironmentController& environment,
    SceneObjectController& sceneObjects,
    Ring* ring,
    Cylinder* cylinder,
    Primitive* primitive
) {
#ifdef USE_IMGUI
    ImGui::Begin(
        "Debug Tools",
        nullptr,
        ImGuiWindowFlags_AlwaysVerticalScrollbar
    );

    DrawSceneControl(
        drawMode,
        hitEffect,
        animationDebug
    );

    if (ImGui::BeginTabBar("DebugToolTabs")) {
        if (ImGui::BeginTabItem("Animation")) {
            animationDebug.DrawImGui();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Effect")) {
            selected_ = EditorSelection::Effects;

            DrawEffectTab(
                context,
                hitEffect,
                ring,
                cylinder,
                primitive
            );

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("PostEffect")) {
            selected_ = EditorSelection::PostEffect;

            DrawPostEffectTab(
                context
            );

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Environment")) {
            selected_ = EditorSelection::Environment;

            DrawEnvironmentTab(
                context->GetObject3dCommon(),
                environment,
                sceneObjects,
                animationDebug
            );

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Sound")) {
            selected_ = EditorSelection::Sound;

            soundController.DrawImGui();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
#endif
}

void SceneDebugPanel::DrawPostEffectTab(
    EngineContext* context
) {
#ifdef USE_IMGUI
    if (!context) {
        return;
    }

    DirectXCommon* dxCommon =
        context->GetDxCommon();

    if (!dxCommon) {
        return;
    }

    auto& vignette =
        dxCommon->GetVignetteSettings();

    auto& smoothing =
        dxCommon->GetSmoothingSettings();

    auto& gaussian =
        dxCommon->GetGaussianSettings();

    auto& radialBlur =
        dxCommon->GetRadialBlurSettings();

    auto& outline =
        dxCommon->GetOutlineSettings();

    auto& dissolve =
        dxCommon->GetDissolveSettings();

    auto& random =
        dxCommon->GetRandomSettings();

    Camera* camera =
        context->GetCamera();

    if (camera) {
        outline.nearClip =
            camera->GetNearClip();

        outline.farClip =
            camera->GetFarClip();
    }

    ImGui::Text("Post Effect Settings");
    ImGui::Separator();

    //====================
    // GrayScale
    //====================

    bool enableGrayScale =
        dxCommon->GetGrayScale();

    if (ImGui::Checkbox(
        "GrayScale",
        &enableGrayScale
    )) {
        dxCommon->SetGrayScale(
            enableGrayScale
        );

        if (enableGrayScale) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            radialBlur.enabled = false;
            outline.enabled = false;
        }
    }

    //====================
    // Vignetting
    //====================

    ImGui::SeparatorText("Vignetting");

    if (ImGui::Checkbox(
        "Enable Vignetting",
        &vignette.enabled
    )) {
        if (vignette.enabled) {
            smoothing.enabled = false;
            gaussian.enabled = false;
            radialBlur.enabled = false;
            outline.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !vignette.enabled
    );

    ImGui::SliderFloat(
        "Vignette Intensity",
        &vignette.intensity,
        0.0f,
        1.0f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Vignette Radius",
        &vignette.radius,
        0.0f,
        1.5f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Vignette Softness",
        &vignette.softness,
        0.01f,
        1.0f,
        "%.2f"
    );

    if (ImGui::Button(
        "Reset Vignetting"
    )) {
        const bool wasEnabled =
            vignette.enabled;

        vignette =
            DirectXCommon::VignetteSettings{};

        vignette.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();

    //====================
    // Smoothing
    //====================

    ImGui::SeparatorText("Smoothing");

    if (ImGui::Checkbox(
        "Enable Smoothing",
        &smoothing.enabled
    )) {
        if (smoothing.enabled) {
            vignette.enabled = false;
            gaussian.enabled = false;
            radialBlur.enabled = false;
            outline.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !smoothing.enabled
    );

    ImGui::SliderInt(
        "Blur Radius",
        &smoothing.radius,
        1,
        4
    );

    ImGui::SliderFloat(
        "Blur Strength",
        &smoothing.strength,
        0.0f,
        1.0f,
        "%.2f"
    );

    const int smoothingKernelSize =
        smoothing.radius * 2 + 1;

    ImGui::Text(
        "Kernel : %d x %d",
        smoothingKernelSize,
        smoothingKernelSize
    );

    if (ImGui::Button(
        "Reset Smoothing"
    )) {
        const bool wasEnabled =
            smoothing.enabled;

        smoothing =
            DirectXCommon::SmoothingSettings{};

        smoothing.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();

    //====================
    // Gaussian Filter
    //====================

    ImGui::SeparatorText(
        "Gaussian Filter"
    );

    if (ImGui::Checkbox(
        "Enable Gaussian Filter",
        &gaussian.enabled
    )) {
        if (gaussian.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            radialBlur.enabled = false;
            outline.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !gaussian.enabled
    );

    ImGui::SliderInt(
        "Gaussian Radius",
        &gaussian.radius,
        1,
        4
    );

    ImGui::SliderFloat(
        "Gaussian Sigma",
        &gaussian.sigma,
        0.1f,
        10.0f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Gaussian Strength",
        &gaussian.strength,
        0.0f,
        1.0f,
        "%.2f"
    );

    const int gaussianKernelSize =
        gaussian.radius * 2 + 1;

    ImGui::Text(
        "Gaussian Kernel : %d x %d",
        gaussianKernelSize,
        gaussianKernelSize
    );

    if (ImGui::Button(
        "Reset Gaussian"
    )) {
        const bool wasEnabled =
            gaussian.enabled;

        gaussian =
            DirectXCommon::GaussianSettings{};

        gaussian.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();

    //====================
    // Radial Blur
    //====================

    ImGui::SeparatorText("Radial Blur");

    if (ImGui::Checkbox(
        "Enable Radial Blur",
        &radialBlur.enabled
    )) {
        if (radialBlur.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            outline.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !radialBlur.enabled
    );

    ImGui::SliderFloat2(
        "Blur Center (UV)",
        radialBlur.center,
        0.0f,
        1.0f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Radial Blur Width",
        &radialBlur.blurWidth,
        0.0f,
        0.1f,
        "%.4f"
    );

    ImGui::SliderFloat(
        "Radial Blur Strength",
        &radialBlur.strength,
        0.0f,
        1.0f,
        "%.2f"
    );

    ImGui::SliderInt(
        "Radial Samples",
        &radialBlur.sampleCount,
        2,
        32
    );

    ImGui::TextDisabled(
        "Center: (0,0) top-left / "
        "(1,1) bottom-right"
    );

    if (ImGui::Button(
        "Reset Radial Blur"
    )) {
        const bool wasEnabled =
            radialBlur.enabled;

        radialBlur =
            DirectXCommon::RadialBlurSettings{};

        radialBlur.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();

    //====================
    // Depth Based Outline
    //====================

    ImGui::SeparatorText(
        "Depth Based Outline"
    );

    if (ImGui::Checkbox(
        "Enable Depth Outline",
        &outline.enabled
    )) {
        if (outline.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            radialBlur.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !outline.enabled
    );

    ImGui::ColorEdit4(
        "Outline Color",
        outline.color
    );

    ImGui::SliderFloat(
        "Outline Threshold",
        &outline.threshold,
        0.001f,
        1.0f,
        "%.3f"
    );

    ImGui::SliderFloat(
        "Outline Strength",
        &outline.strength,
        0.0f,
        5.0f,
        "%.2f"
    );

    ImGui::SliderInt(
        "Outline Thickness",
        &outline.thickness,
        1,
        4
    );

    ImGui::Text(
        "Near Clip : %.3f",
        outline.nearClip
    );

    ImGui::Text(
        "Far Clip : %.3f",
        outline.farClip
    );

    if (ImGui::Button(
        "Reset Outline"
    )) {
        const bool wasEnabled =
            outline.enabled;

        outline =
            DirectXCommon::OutlineSettings{};

        outline.enabled =
            wasEnabled;

        if (camera) {
            outline.nearClip =
                camera->GetNearClip();

            outline.farClip =
                camera->GetFarClip();
        }
    }

    ImGui::EndDisabled();


    //====================
    // Dissolve
    //====================

    ImGui::SeparatorText("Dissolve");

    if (ImGui::Checkbox(
        "Enable Dissolve",
        &dissolve.enabled
    )) {
        if (dissolve.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            outline.enabled = false;

            // RadialBlur追加済みの場合
            radialBlur.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !dissolve.enabled
    );

    ImGui::SliderFloat(
        "Dissolve Threshold",
        &dissolve.threshold,
        0.0f,
        1.0f,
        "%.3f"
    );

    ImGui::SliderFloat(
        "Dissolve Edge Width",
        &dissolve.edgeWidth,
        0.001f,
        0.25f,
        "%.3f"
    );

    ImGui::SliderFloat(
        "Dissolve Edge Intensity",
        &dissolve.edgeIntensity,
        0.0f,
        1.0f,
        "%.2f"
    );

    ImGui::ColorEdit4(
        "Dissolve Edge Color",
        dissolve.edgeColor
    );

    if (ImGui::Button(
        "Reset Dissolve"
    )) {
        const bool wasEnabled =
            dissolve.enabled;

        dissolve =
            DirectXCommon::DissolveSettings{};

        dissolve.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();


    //====================
    //RandomNoise
    //====================

    ImGui::SeparatorText("Random Noise");

    if (ImGui::Checkbox(
        "Enable Random Noise",
        &random.enabled
    )) {
        if (random.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            outline.enabled = false;

            // 追加済みの場合
            radialBlur.enabled = false;
            dissolve.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !random.enabled
    );

    ImGui::Checkbox(
        "Animate Random Noise",
        &random.animate
    );

    ImGui::Checkbox(
        "Show Noise Only",
        &random.showNoiseOnly
    );

    ImGui::SliderFloat(
        "Random Speed",
        &random.speed,
        0.0f,
        10.0f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Random Scale",
        &random.scale,
        1.0f,
        2000.0f,
        "%.0f"
    );

    ImGui::SliderFloat(
        "Random Strength",
        &random.strength,
        0.0f,
        1.0f,
        "%.2f"
    );

    if (ImGui::Button(
        "Reset Random Settings"
    )) {
        const bool wasEnabled =
            random.enabled;

        random =
            DirectXCommon::RandomSettings{};

        random.enabled =
            wasEnabled;

        dxCommon->ResetRandomTime();
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Reset Random Time"
    )) {
        dxCommon->ResetRandomTime();
    }

    ImGui::EndDisabled();

    //====================
    // Current Effect
    //====================

    ImGui::SeparatorText(
        "Current Post Effect"
    );

    const char* currentEffect = "None";

    if (outline.enabled) {
        currentEffect =
            "Depth Based Outline";
    } else if (random.enabled) {
        currentEffect =
            "Random Noise";
    } else if (dissolve.enabled) {
        currentEffect =
            "Dissolve";
    } else if (radialBlur.enabled) {
        currentEffect =
            "Radial Blur";
    } else if (gaussian.enabled) {
        currentEffect =
            "Gaussian Filter";
    } else if (smoothing.enabled) {
        currentEffect =
            "Smoothing";
    } else if (vignette.enabled) {
        currentEffect =
            "Vignetting";
    } else if (dxCommon->GetGrayScale()) {
        currentEffect =
            "GrayScale";
    }

    ImGui::Text(
        "Active : %s",
        currentEffect
    );

    if (ImGui::Button(
        "Disable All Post Effects"
    )) {
        vignette.enabled = false;
        smoothing.enabled = false;
        gaussian.enabled = false;
        radialBlur.enabled = false;
        outline.enabled = false;
        dissolve.enabled = false;
        random.enabled = false;

        dxCommon->SetGrayScale(false);
    }
#endif
}


void SceneDebugPanel::DrawEnvironmentTab(
    Object3dCommon* object3dCommon,
    EnvironmentController& environment,
    SceneObjectController& sceneObjects,
    AnimationDebugController& animationDebug
) {
#ifdef USE_IMGUI
    ImGui::Checkbox(
        "Show Terrain",
        &sceneObjects.ShowTerrain()
    );

    ImGui::Checkbox(
        "Show Axis",
        &sceneObjects.ShowAxis()
    );

    if (environment.DrawImGui()) {
        sceneObjects.SetEnvironmentCoefficient(
            environment.GetEnvironmentCoefficient()
        );

        animationDebug.SetEnvironmentCoefficient(
            environment.GetEnvironmentCoefficient()
        );
    }

    if (!object3dCommon) {
        return;
    }


    ImGui::SeparatorText("Light Settings");

    // ==================================================
    // DirectionalLight
    // ==================================================

    if (ImGui::CollapsingHeader(
        "Directional Light",
        ImGuiTreeNodeFlags_DefaultOpen
    )) {
        DirectionalLight& light =
            object3dCommon->GetDirectionalLight();

        ImGui::ColorEdit4(
            "Directional Color",
            &light.color.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        if (ImGui::DragFloat3(
            "Directional Direction",
            &light.direction.x,
            0.01f,
            -1.0f,
            1.0f
        )) {
            float length = std::sqrt(
                light.direction.x * light.direction.x +
                light.direction.y * light.direction.y +
                light.direction.z * light.direction.z
            );

            if (length > 0.0001f) {
                light.direction.x /= length;
                light.direction.y /= length;
                light.direction.z /= length;
            }
        }

        ImGui::DragFloat(
            "Directional Intensity",
            &light.intensity,
            0.01f,
            0.0f,
            20.0f
        );

        if (ImGui::Button("Reset Directional Light")) {
            light.color =
            { 1.0f, 1.0f, 1.0f, 1.0f };

            light.direction =
            { 0.0f, -1.0f, 0.0f };

            light.intensity = 0.2f;
        }
    }

    // ==================================================
    // SpotLight
    // ==================================================

    if (ImGui::CollapsingHeader(
        "Spot Light",
        ImGuiTreeNodeFlags_DefaultOpen
    )) {
        SpotLight& light =
            object3dCommon->GetSpotLight();

        ImGui::ColorEdit4(
            "Spot Color",
            &light.color.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::DragFloat3(
            "Spot Position",
            &light.position.x,
            0.05f,
            -100.0f,
            100.0f
        );

        if (ImGui::DragFloat3(
            "Spot Direction",
            &light.direction.x,
            0.01f,
            -1.0f,
            1.0f
        )) {
            float length = std::sqrt(
                light.direction.x * light.direction.x +
                light.direction.y * light.direction.y +
                light.direction.z * light.direction.z
            );

            if (length > 0.0001f) {
                light.direction.x /= length;
                light.direction.y /= length;
                light.direction.z /= length;
            }
        }

        ImGui::DragFloat(
            "Spot Intensity",
            &light.intensity,
            0.05f,
            0.0f,
            100.0f
        );

        ImGui::DragFloat(
            "Spot Distance",
            &light.distance,
            0.1f,
            0.1f,
            200.0f
        );

        ImGui::DragFloat(
            "Spot Decay",
            &light.decay,
            0.01f,
            0.01f,
            10.0f
        );

        constexpr float kPi =
            3.14159265358979323846f;

        // shaderにはcos値を渡しているため、
        // ImGuiでは扱いやすい角度へ一度戻す
        float outerAngle =
            std::acos(std::clamp(
                light.cosAngle,
                -1.0f,
                1.0f
            )) * 180.0f / kPi;

        float innerAngle =
            std::acos(std::clamp(
                light.cosFalloffStart,
                -1.0f,
                1.0f
            )) * 180.0f / kPi;

        bool angleChanged = false;

        angleChanged |= ImGui::SliderFloat(
            "Spot Inner Angle",
            &innerAngle,
            0.0f,
            89.0f,
            "%.1f deg"
        );

        angleChanged |= ImGui::SliderFloat(
            "Spot Outer Angle",
            &outerAngle,
            1.0f,
            89.0f,
            "%.1f deg"
        );

        if (angleChanged) {
            // 内側角度が外側を超えないようにする
            innerAngle = std::clamp(
                innerAngle,
                0.0f,
                outerAngle
            );

            outerAngle = std::clamp(
                outerAngle,
                innerAngle,
                89.0f
            );

            light.cosAngle =
                std::cos(
                    outerAngle * kPi / 180.0f
                );

            light.cosFalloffStart =
                std::cos(
                    innerAngle * kPi / 180.0f
                );
        }

        ImGui::Spacing();

        ImGui::TextDisabled(
            "Inner: %.1f deg / Outer: %.1f deg",
            innerAngle,
            outerAngle
        );

        ImGui::TextDisabled(
            "Cos Inner: %.3f / Cos Outer: %.3f",
            light.cosFalloffStart,
            light.cosAngle
        );

        if (ImGui::Button("Reset Spot Light")) {
            light.color =
            { 1.0f, 1.0f, 1.0f, 1.0f };

            light.position =
            { 0.0f, 5.0f, 0.0f };

            light.direction =
            { 0.0f, -1.0f, 0.0f };

            light.intensity = 4.0f;
            light.distance = 15.0f;
            light.decay = 2.0f;

            light.cosAngle =
                std::cos(
                    30.0f * kPi / 180.0f
                );

            light.cosFalloffStart =
                std::cos(
                    20.0f * kPi / 180.0f
                );

            light.padding = 0.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Turn Off Spot Light")) {
            light.intensity = 0.0f;
        }
    }


    //===================================
    //PointLight
    //===================================
    if (ImGui::CollapsingHeader(
        "Point Light",
        ImGuiTreeNodeFlags_DefaultOpen
    )) {
        PointLight& light =
            object3dCommon->GetPointLight();

        ImGui::ColorEdit4(
            "Point Color",
            &light.color.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::DragFloat3(
            "Point Position",
            &light.position.x,
            0.05f,
            -100.0f,
            100.0f
        );

        ImGui::DragFloat(
            "Point Intensity",
            &light.intensity,
            0.05f,
            0.0f,
            100.0f
        );

        ImGui::DragFloat(
            "Point Radius",
            &light.radius,
            0.1f,
            0.1f,
            200.0f
        );

        ImGui::DragFloat(
            "Point Decay",
            &light.decay,
            0.01f,
            0.01f,
            10.0f
        );

        if (ImGui::Button("Reset Point Light")) {
            light.color =
            { 1.0f, 1.0f, 1.0f, 1.0f };

            light.position =
            { 0.0f, 3.0f, 0.0f };

            light.intensity = 2.0f;
            light.radius = 10.0f;
            light.decay = 2.0f;

            light.padding[0] = 0.0f;
            light.padding[1] = 0.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Turn Off Point Light")) {
            light.intensity = 0.0f;
        }
    } // Point LightのCollapsingHeader

#endif
} // DrawEnvironmentTab


void SceneDebugPanel::DrawGameViewWindow(
    EngineContext* context,
    SceneObjectController& sceneObjects
)
{
#ifdef USE_IMGUI
    ImGui::Begin("Game View");

    if (!context || !context->GetImGuiManager()) {
        ImGui::TextDisabled("Game View unavailable.");
        ImGui::End();
        return;
    }

    ImGui::Checkbox("Gizmo", &showTransformGizmo_);

    ImGui::SameLine();
    ImGui::RadioButton("Move", &gizmoOperation_, 0);

    ImGui::SameLine();
    ImGui::RadioButton("Rotate", &gizmoOperation_, 1);

    ImGui::SameLine();
    ImGui::RadioButton("Scale", &gizmoOperation_, 2);

    ImGui::SameLine();
    ImGui::RadioButton("Local", &gizmoMode_, 0);

    ImGui::SameLine();
    ImGui::RadioButton("World", &gizmoMode_, 1);

    ImGuiManager* imguiManager =
        context->GetImGuiManager();

    imguiManager->UpdateGameViewTexture();

    ImVec2 availableSize =
        ImGui::GetContentRegionAvail();

    if (availableSize.x <= 1.0f || availableSize.y <= 1.0f) {
        ImGui::TextDisabled("No space for Game View.");
        ImGui::End();
        return;
    }

    const uint32_t srvIndex =
        imguiManager->GetGameViewSrvIndex();

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle =
        context->GetSrvManager()->GetGPUDescriptorHandle(srvIndex);

    ImTextureID textureId =
        static_cast<ImTextureID>(gpuHandle.ptr);

    ImGui::Image(
        textureId,
        availableSize,
        ImVec2(0.0f, 0.0f),
        ImVec2(1.0f, 1.0f)
    );

    const ImVec2 imageMin = ImGui::GetItemRectMin();
    const ImVec2 imageMax = ImGui::GetItemRectMax();

    DrawTransformGizmo(
        context,
        sceneObjects,
        imageMin.x,
        imageMin.y,
        imageMax.x - imageMin.x,
        imageMax.y - imageMin.y
    );

    ImGui::End();
#endif
}


void SceneDebugPanel::DrawTransformGizmo(
    EngineContext* context,
    SceneObjectController& sceneObjects,
    float rectX,
    float rectY,
    float rectWidth,
    float rectHeight
)
{
#ifdef USE_IMGUI
    if (!showTransformGizmo_) {
        return;
    }

    if (!context || !context->GetCamera()) {
        return;
    }

    Object3d* object = nullptr;

    switch (selected_) {
    case EditorSelection::Terrain:
        object = sceneObjects.GetEditorObject(
            SceneObjectController::EditorObjectType::Terrain
        );
        break;

    case EditorSelection::AxisPositive:
        object = sceneObjects.GetEditorObject(
            SceneObjectController::EditorObjectType::AxisPositive
        );
        break;

    case EditorSelection::AxisNegative:
        object = sceneObjects.GetEditorObject(
            SceneObjectController::EditorObjectType::AxisNegative
        );
        break;

    case EditorSelection::DynamicObject:
        object = sceneObjects.GetObject(selectedObjectIndex_);
        break;

    default:
        break;
    }

    if (!object) {
        return;
    }

    Camera* camera =
        context->GetCamera();

    Transform& transform =
        object->GetTransform();

    Matrix4x4 worldMatrix =
        Math::MakeAffineMatrix(
            transform.scale,
            transform.rotate,
            transform.translate
        );

    float world[16]{};
    float view[16]{};
    float projection[16]{};

    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            const int index = row * 4 + column;

            world[index] =
                worldMatrix.m[row][column];

            view[index] =
                camera->GetViewMatrix().m[row][column];

            projection[index] =
                camera->GetProjectionMatrix().m[row][column];
        }
    }

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();

    ImGuizmo::SetRect(
        rectX,
        rectY,
        rectWidth,
        rectHeight
    );

    ImGuizmo::OPERATION operation =
        ImGuizmo::TRANSLATE;

    if (gizmoOperation_ == 1) {
        operation = ImGuizmo::ROTATE;
    } else if (gizmoOperation_ == 2) {
        operation = ImGuizmo::SCALE;
    }

    ImGuizmo::MODE mode =
        gizmoMode_ == 0
        ? ImGuizmo::LOCAL
        : ImGuizmo::WORLD;

    ImGuizmo::Manipulate(
        view,
        projection,
        operation,
        mode,
        world
    );

    if (ImGuizmo::IsUsing()) {
        float translate[3]{};
        float rotateDegrees[3]{};
        float scale[3]{};

        ImGuizmo::DecomposeMatrixToComponents(
            world,
            translate,
            rotateDegrees,
            scale
        );

        transform.translate.x = translate[0];
        transform.translate.y = translate[1];
        transform.translate.z = translate[2];

        transform.rotate.x = rotateDegrees[0] * 0.017453292519943295f;
        transform.rotate.y = rotateDegrees[1] * 0.017453292519943295f;
        transform.rotate.z = rotateDegrees[2] * 0.017453292519943295f;

        transform.scale.x = scale[0];
        transform.scale.y = scale[1];
        transform.scale.z = scale[2];
    }
#endif
}


void SceneDebugPanel::DrawSceneControl(GameSceneDrawMode& drawMode, HitEffectController& hitEffect, AnimationDebugController& animationDebug)
{

#ifdef USE_IMGUI
    ImGui::Text("Scene Control");
    ImGui::Separator();

    if (ImGui::Button("Emit Effect SPACE", ImVec2(180.0f, 32.0f))) {
        hitEffect.Emit({ 0.0f, 3.0f, 0.0f });
        AddEditorLog(EditorLogType::Info, "Effect emitted from Debug Tools.");
    }

    ImGui::SameLine();
    ImGui::TextDisabled("SPACE key also emits");

    ImGui::Spacing();

    ImGui::Text("Draw Mode");
    ImGui::Separator();

    int currentMode = static_cast<int>(drawMode);

    if (ImGui::RadioButton("Normal OBJ / Terrain", currentMode == 0)) {
        drawMode = GameSceneDrawMode::NormalObj;
    }

    if (ImGui::RadioButton("Animation glTF", currentMode == 1)) {
        drawMode = GameSceneDrawMode::Animation;
    }

    ImGui::Spacing();

    ImGui::Text("Skeleton");
    ImGui::BulletText("Joint Count : %d", animationDebug.GetJointCount());
    ImGui::BulletText("Animation Time : %.2f", animationDebug.GetAnimationTime());

    ImGui::Spacing();

    ImGui::Checkbox(
        "Show Skeleton Debug",
        &animationDebug.ShowSkeletonDebug()
    );
#endif

}

void SceneDebugPanel::DrawEffectTab(
    EngineContext* context,
    HitEffectController& hitEffect,
    Ring* ring,
    Cylinder* cylinder,
    Primitive* primitive
) {



#ifdef USE_IMGUI
    ImGui::Checkbox(
        "Enable Primitive",
        &hitEffect.EnablePrimitive()
    );

    ImGui::Checkbox(
        "Enable Ring",
        &hitEffect.EnableRing()
    );

    ImGui::Checkbox(
        "Enable Cylinder",
        &hitEffect.EnableCylinder()
    );

    GPUParticleManager* gpuParticleManager =
        context->GetGPUParticleManager();

    if (gpuParticleManager)
    {
        GPUParticleManager::Settings& settings =
            gpuParticleManager->GetSettings();

        ImGui::SeparatorText("GPU Particle");

        ImGui::Checkbox(
            "Enable GPU Particle",
            &settings.enabled
        );

        ImGui::DragInt(
            "GPU Fire Count",
            &settings.fireCount,
            1.0f,
            1,
            static_cast<int>(
                GPUParticleManager::kMaxParticles
                )
        );

        ImGui::DragInt(
            "GPU Sakura Count",
            &settings.sakuraCount,
            1.0f,
            1,
            static_cast<int>(
                GPUParticleManager::kMaxParticles
                )
        );

        ImGui::DragFloat(
            "GPU Particle Scale",
            &settings.particleScale,
            0.01f,
            0.01f,
            10.0f,
            "%.2f"
        );

        ImGui::DragFloat(
            "GPU Spawn Radius",
            &settings.spawnRadius,
            0.01f,
            0.0f,
            20.0f,
            "%.2f"
        );

        ImGui::ColorEdit4(
            "GPU Fire Main Color",
            &settings.fireMainColor.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::ColorEdit4(
            "GPU Fire Sub Color",
            &settings.fireSubColor.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::ColorEdit4(
            "GPU Sakura Main Color",
            &settings.sakuraMainColor.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::ColorEdit4(
            "GPU Sakura Sub Color",
            &settings.sakuraSubColor.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        if (ImGui::Button("Reset GPU Particle"))
        {
            settings =
                GPUParticleManager::Settings{};
        }

        ImGui::SameLine();


        ImGui::Text(
            "GPU Particle : %s",
            settings.enabled ? "ON" : "OFF"
        );

        ImGui::Text(
            "Emit Position : %.2f, %.2f, %.2f",
            hitEffect.GetPosition().x,
            hitEffect.GetPosition().y,
            hitEffect.GetPosition().z
        );

        ImGui::Text(
            "Fire Count : %d",
            settings.fireCount
        );

        if (ImGui::Button("Preview GPU Particle"))
        {
            // OFFだった場合もPreviewではONにする
            settings.enabled = true;

            const Vector3 emitPosition =
                hitEffect.GetPosition();

            const float emitSize =
                (std::max)(
                    hitEffect.GetSize(),
                    0.01f
                    );

            if (
                hitEffect.GetType() ==
                HitEffectController::Type::Sakura
                ) {
                settings.sakuraCount =
                    std::clamp(
                        settings.sakuraCount,
                        1,
                        static_cast<int>(
                            GPUParticleManager::kMaxParticles
                            )
                    );

                gpuParticleManager->EmitSakura(
                    emitPosition,
                    static_cast<uint32_t>(
                        settings.sakuraCount
                        ),
                    emitSize
                );
            } else
            {
                settings.fireCount =
                    std::clamp(
                        settings.fireCount,
                        1,
                        static_cast<int>(
                            GPUParticleManager::kMaxParticles
                            )
                    );

                gpuParticleManager->Emit(
                    emitPosition,
                    static_cast<uint32_t>(
                        settings.fireCount
                        ),
                    emitSize
                );
            }
        }
    }

    ImGui::SeparatorText("Hit Effect Type");

    if (ImGui::RadioButton(
        "Fire",
        hitEffect.GetType() == HitEffectController::Type::Fire
    )) {
        hitEffect.GetType() =
            HitEffectController::Type::Fire;
    }

    ImGui::SameLine();

    if (ImGui::RadioButton(
        "Sakura",
        hitEffect.GetType() == HitEffectController::Type::Sakura
    )) {
        hitEffect.GetType() =
            HitEffectController::Type::Sakura;
    }

    ImGui::SliderFloat(
        "Hit Effect Size",
        &hitEffect.GetSize(),
        0.1f,
        5.0f,
        "%.2f"
    );

    if (
        hitEffect.GetType() ==
        HitEffectController::Type::Sakura
        ) {
        ParticleManager::SakuraSettings& settings =
            context
            ->GetParticleManager()
            ->GetSakuraSettings();

        ImGui::SeparatorText("Sakura Settings");

        ImGui::ColorEdit4(
            "Sakura Main Color",
            &settings.color.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::ColorEdit4(
            "Sakura Sub Color",
            &settings.subColor.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::DragInt(
            "Petal Count",
            &settings.petalCount,
            1.0f,
            1,
            256
        );

        ImGui::DragFloatRange2(
            "Petal Size",
            &settings.minSize,
            &settings.maxSize,
            0.005f,
            0.01f,
            2.0f
        );

        ImGui::DragFloat(
            "Spawn Radius",
            &settings.spawnRadius,
            0.01f,
            0.0f,
            10.0f
        );

        ImGui::DragFloat(
            "Spread Speed",
            &settings.spreadSpeed,
            0.001f,
            0.0f,
            1.0f
        );

        ImGui::DragFloat(
            "Upward Speed",
            &settings.upwardSpeed,
            0.001f,
            -1.0f,
            1.0f
        );

        ImGui::DragFloatRange2(
            "Petal Life Time",
            &settings.minLifeTime,
            &settings.maxLifeTime,
            0.01f,
            0.05f,
            10.0f
        );

        ImGui::DragFloat(
            "Petal Gravity",
            &settings.gravity,
            0.0001f,
            -0.1f,
            0.1f,
            "%.4f"
        );

        ImGui::DragFloat(
            "Petal Rotation",
            &settings.rotationSpeed,
            0.005f,
            0.0f,
            1.0f
        );

        ImGui::DragFloat(
            "Flash Size",
            &settings.flashSize,
            0.01f,
            0.01f,
            10.0f
        );

        ImGui::DragFloat(
            "Flash Life Time",
            &settings.flashLifeTime,
            0.01f,
            0.05f,
            5.0f
        );

        if (ImGui::Button("Reset Sakura Settings")) {
            settings =
                ParticleManager::SakuraSettings{};
        }
    }

    ImGui::SeparatorText("Emit");

    ImGui::DragFloat3(
        "Effect Position",
        &hitEffect.GetPosition().x,
        0.05f,
        -20.0f,
        20.0f
    );

    ImGui::SeparatorText("Hit Effect Size");

    ImGui::SliderFloat(
        "Effect Size",
        &hitEffect.GetSize(),
        0.1f,
        5.0f,
        "%.2f"
    );

    if (ImGui::Button("Reset Effect Size")) {
        hitEffect.GetSize() = 1.0f;
    }

    ImGui::SameLine();

    ImGui::TextDisabled(
        "Current : %.2f x",
        hitEffect.GetSize()
    );

    ImGui::SeparatorText("Effect Presets");

    ImGui::InputText(
        "Preset Name",
        hitEffect.GetPresetNameBuffer().data(),
        hitEffect.GetPresetNameBuffer().size()
    );

    if (ImGui::Button(
        "Save Current Preset",
        ImVec2(180.0f, 30.0f)
    )) {
        hitEffect.SavePreset(
            hitEffect.GetPresetNameBuffer().data()
        );
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Refresh Presets",
        ImVec2(160.0f, 30.0f)
    )) {
        hitEffect.RefreshPresetList();
    }

    if (!hitEffect.GetPresetNames().empty()) {
        hitEffect.GetSelectedPreset() =
            std::clamp(
                hitEffect.GetSelectedPreset(),
                0,
                static_cast<int>(
                    hitEffect.GetPresetNames().size()
                    ) - 1
            );

        const char* previewName =
            hitEffect.GetPresetNames()[
                hitEffect.GetSelectedPreset()
            ].c_str();

        if (ImGui::BeginCombo(
            "Saved Presets",
            previewName
        )) {
            for (
                int index = 0;
                index <
                static_cast<int>(
                    hitEffect.GetPresetNames().size()
                    );
                    ++index
                ) {
                const bool selected =
                    hitEffect.GetSelectedPreset() ==
                    index;

                if (ImGui::Selectable(
                    hitEffect.GetPresetNames()[index].c_str(),
                    selected
                )) {
                    hitEffect.GetSelectedPreset() =
                        index;
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        if (ImGui::Button(
            "Load Selected",
            ImVec2(180.0f, 30.0f)
        )) {
            hitEffect.LoadPreset(
                hitEffect.GetPresetNames()[
                    hitEffect.GetSelectedPreset()
                ]
            );
        }

        ImGui::SameLine();

        if (ImGui::Button(
            "Load And Preview",
            ImVec2(180.0f, 30.0f)
        )) {
            if (hitEffect.LoadPreset(
                hitEffect.GetPresetNames()[
                    hitEffect.GetSelectedPreset()
                ]
            )) {
                hitEffect.Emit(
                    hitEffect.GetPosition()
                );
            }
        }
    } else {
        ImGui::TextDisabled(
            "No saved presets"
        );
    }

    if (!hitEffect.GetMessage().empty()) {
        ImGui::TextDisabled(
            "%s",
            hitEffect.GetMessage().c_str()
        );
    }

    ImGui::SeparatorText("Preview");

    if (ImGui::Button(
        "Preview Current",
        ImVec2(180.0f, 32.0f)
    )) {
        hitEffect.Emit(
            hitEffect.GetPosition()
        );
    }

    if (ImGui::Button(
        "Apply Fire Preset",
        ImVec2(180.0f, 30.0f)
    )) {
        hitEffect.GetType() =
            HitEffectController::Type::Fire;

        hitEffect.ApplyFirePreset();
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Apply Fire And Preview",
        ImVec2(180.0f, 30.0f)
    )) {
        hitEffect.GetType() =
            HitEffectController::Type::Fire;

        hitEffect.ApplyFirePreset();

        hitEffect.Emit(
            hitEffect.GetPosition()
        );
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Preview Fire",
        ImVec2(180.0f, 30.0f)
    )) {
        hitEffect.Emit(hitEffect.GetPosition());
    }

    if (ring) {
        Ring::Settings& settings =
            ring->GetSettings();

        ImGui::SeparatorText("Ring");

        ImGui::ColorEdit4(
            "Ring Start Color",
            &settings.color.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::ColorEdit4(
            "Ring End Color",
            &settings.endColor.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::DragFloat(
            "Ring Glow Intensity",
            &settings.intensity,
            0.01f,
            0.0f,
            10.0f
        );

        ImGui::DragFloat(
            "Ring Start Scale",
            &settings.startScale,
            0.01f,
            0.01f,
            10.0f
        );

        ImGui::DragFloat(
            "Ring End Scale",
            &settings.endScale,
            0.01f,
            0.01f,
            20.0f
        );

        ImGui::SliderFloat(
            "Ring Start Thickness",
            &settings.thickness,
            0.001f,
            0.5f
        );

        ImGui::SliderFloat(
            "Ring End Thickness",
            &settings.endThickness,
            0.001f,
            0.5f
        );

        ImGui::DragFloat(
            "Ring Life Time",
            &settings.lifeTime,
            0.01f,
            0.05f,
            5.0f
        );

        ImGui::SliderFloat(
            "Ring Fade In Ratio",
            &settings.fadeInRatio,
            0.0f,
            1.0f
        );

        ImGui::DragFloat(
            "Ring Ease Power",
            &settings.easePower,
            0.01f,
            0.01f,
            10.0f
        );

        ImGui::DragFloat(
            "Ring Rotation Speed",
            &settings.rotationSpeed,
            0.01f,
            -20.0f,
            20.0f
        );

        ImGui::DragFloat(
            "Ring Edge Softness",
            &settings.edgeSoftness,
            0.001f,
            0.001f,
            0.25f
        );

        ImGui::DragFloat(
            "Ring Glow Strength",
            &settings.glowStrength,
            0.01f,
            0.0f,
            5.0f
        );

        ImGui::DragFloat(
            "Ring Distortion",
            &settings.distortionStrength,
            0.001f,
            0.0f,
            0.5f
        );

        ImGui::DragFloat(
            "Ring Distortion Frequency",
            &settings.distortionFrequency,
            0.1f,
            1.0f,
            32.0f
        );

        ImGui::DragFloat(
            "Ring Distortion Speed",
            &settings.distortionSpeed,
            0.01f,
            -10.0f,
            10.0f
        );

        if (ImGui::Button("Reset Ring")) {
            settings =
                Ring::Settings{};
        }
    }

    if (primitive) {
        Primitive::Settings& settings =
            primitive->GetSettings();

        ImGui::SeparatorText("Primitive");

        ImGui::ColorEdit4(
            "Primitive Start Color",
            &settings.color.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::ColorEdit4(
            "Primitive End Color",
            &settings.endColor.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::DragInt(
            "Primitive Count",
            &settings.count,
            1.0f,
            1,
            128
        );

        ImGui::DragFloat(
            "Primitive Glow",
            &settings.intensity,
            0.01f,
            0.0f,
            10.0f
        );

        ImGui::DragFloat(
            "Primitive Width",
            &settings.width,
            0.001f,
            0.001f,
            2.0f
        );

        ImGui::SliderFloat(
            "Primitive Width Random",
            &settings.widthRandomness,
            0.0f,
            1.0f
        );

        ImGui::DragFloatRange2(
            "Primitive Length",
            &settings.minLength,
            &settings.maxLength,
            0.01f,
            0.01f,
            10.0f
        );

        ImGui::DragFloatRange2(
            "Primitive Life Time",
            &settings.minLifeTime,
            &settings.maxLifeTime,
            0.01f,
            0.01f,
            5.0f
        );

        ImGui::DragFloat(
            "Primitive Move Speed",
            &settings.moveSpeed,
            0.01f,
            0.0f,
            20.0f
        );

        ImGui::SliderFloat(
            "Primitive Speed Random",
            &settings.moveSpeedRandomness,
            0.0f,
            1.0f
        );

        ImGui::DragFloat(
            "Primitive Rotation Speed",
            &settings.rotationSpeed,
            0.01f,
            -20.0f,
            20.0f
        );

        ImGui::DragFloat(
            "Primitive Rotation Random",
            &settings.rotationSpeedRandomness,
            0.01f,
            0.0f,
            20.0f
        );

        ImGui::SliderAngle(
            "Primitive Direction",
            &settings.directionAngle,
            -180.0f,
            180.0f
        );

        ImGui::SliderAngle(
            "Primitive Direction Spread",
            &settings.directionSpread,
            0.0f,
            180.0f
        );

        ImGui::DragFloat(
            "Primitive Spawn Radius",
            &settings.spawnRadius,
            0.01f,
            0.0f,
            10.0f
        );

        ImGui::DragFloat3(
            "Primitive Acceleration",
            &settings.acceleration.x,
            0.01f,
            -20.0f,
            20.0f
        );

        ImGui::DragFloat(
            "Primitive End Width Scale",
            &settings.endWidthScale,
            0.01f,
            0.0f,
            5.0f
        );

        ImGui::DragFloat(
            "Primitive End Length Scale",
            &settings.endLengthScale,
            0.01f,
            0.0f,
            5.0f
        );

        ImGui::DragFloat(
            "Primitive Scale Ease",
            &settings.scaleEasePower,
            0.01f,
            0.01f,
            10.0f
        );

        ImGui::SliderFloat(
            "Primitive Fade In",
            &settings.fadeInRatio,
            0.0f,
            1.0f
        );

        ImGui::DragFloat(
            "Primitive Fade Power",
            &settings.fadePower,
            0.01f,
            0.01f,
            10.0f
        );

        if (ImGui::Button("Reset Primitive")) {
            settings =
                Primitive::Settings{};
        }
    }

    if (cylinder) {
        Cylinder::Settings& settings =
            cylinder->GetSettings();

        ImGui::SeparatorText("Cylinder");

        ImGui::ColorEdit4(
            "Cylinder Start Color",
            &settings.color.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::ColorEdit4(
            "Cylinder End Color",
            &settings.endColor.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::DragFloat(
            "Cylinder Glow",
            &settings.intensity,
            0.01f,
            0.0f,
            10.0f
        );

        ImGui::DragFloat(
            "Cylinder Start Radius",
            &settings.radius,
            0.01f,
            0.01f,
            10.0f
        );

        ImGui::DragFloat(
            "Cylinder End Radius",
            &settings.endRadius,
            0.01f,
            0.01f,
            10.0f
        );

        ImGui::DragFloat(
            "Cylinder Start Height",
            &settings.startHeight,
            0.01f,
            0.01f,
            20.0f
        );

        ImGui::DragFloat(
            "Cylinder End Height",
            &settings.endHeight,
            0.01f,
            0.01f,
            30.0f
        );

        ImGui::DragFloat(
            "Cylinder Bottom Radius Scale",
            &settings.bottomRadiusScale,
            0.01f,
            0.0f,
            5.0f
        );

        ImGui::DragFloat(
            "Cylinder Top Radius Scale",
            &settings.topRadiusScale,
            0.01f,
            0.0f,
            5.0f
        );

        ImGui::DragFloat(
            "Cylinder Life Time",
            &settings.lifeTime,
            0.01f,
            0.05f,
            10.0f
        );

        ImGui::DragFloat(
            "Cylinder Rise Distance",
            &settings.riseDistance,
            0.01f,
            -10.0f,
            20.0f
        );

        ImGui::DragFloat(
            "Cylinder Ease Power",
            &settings.easePower,
            0.01f,
            0.01f,
            10.0f
        );

        ImGui::SliderFloat(
            "Cylinder Fade In",
            &settings.fadeInRatio,
            0.0f,
            1.0f
        );

        ImGui::DragFloat(
            "Cylinder Fade Power",
            &settings.fadePower,
            0.01f,
            0.01f,
            10.0f
        );

        ImGui::DragFloat(
            "Cylinder Twist Amount",
            &settings.twistAmount,
            0.01f,
            -20.0f,
            20.0f
        );

        ImGui::DragFloat(
            "Cylinder Twist Speed",
            &settings.twistSpeed,
            0.01f,
            -20.0f,
            20.0f
        );

        ImGui::SliderFloat(
            "Cylinder Noise Strength",
            &settings.noiseStrength,
            0.0f,
            1.0f
        );

        ImGui::DragFloat(
            "Cylinder Noise Frequency",
            &settings.noiseFrequency,
            0.1f,
            1.0f,
            32.0f
        );

        ImGui::DragFloat(
            "Cylinder Noise Speed",
            &settings.noiseSpeed,
            0.01f,
            -10.0f,
            10.0f
        );

        ImGui::SliderFloat(
            "Cylinder Top Fade",
            &settings.topFade,
            0.001f,
            1.0f
        );

        ImGui::SliderFloat(
            "Cylinder Bottom Fade",
            &settings.bottomFade,
            0.001f,
            1.0f
        );

        ImGui::DragFloat3(
            "Cylinder Position Offset",
            &settings.positionOffset.x,
            0.01f,
            -20.0f,
            20.0f
        );

        if (ImGui::Button("Reset Cylinder")) {
            settings =
                Cylinder::Settings{};
        }
    }

#endif
}
