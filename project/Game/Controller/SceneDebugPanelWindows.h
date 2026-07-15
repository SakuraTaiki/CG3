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

            if (ImGui::MenuItem("Save Level Scene Json", "Ctrl+S")) {
                if (sceneObjects.SaveLevelSceneToJson("Resources/LevelScene.json")) {
                    Detail::AddEditorLog(
                        Detail::EditorLogType::Info,
                        "Saved level scene: Resources/LevelScene.json"
                    );
                } else {
                    Detail::AddEditorLog(
                        Detail::EditorLogType::Error,
                        "Save level scene failed."
                    );
                }
            }

            if (ImGui::MenuItem("Reload Level Scene Json", "Ctrl+O")) {
                if (sceneObjects.LoadLevelSceneFromJson("Resources/LevelScene.json")) {
                    selected_ = EditorSelection::None;
                    selectedObjectIndex_ = 0;
                    Detail::GetModelApplyTargetIndex() = 0;

                    Detail::AddEditorLog(
                        Detail::EditorLogType::Info,
                        "Reloaded level scene: Resources/LevelScene.json"
                    );
                } else {
                    Detail::AddEditorLog(
                        Detail::EditorLogType::Error,
                        "Reload level scene failed."
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

        if (ImGui::BeginMenu("MyMenu")) {
            const bool hasSelectedObject =
                selected_ == EditorSelection::DynamicObject &&
                selectedObjectIndex_ < sceneObjects.GetObjectCount();

            if (ImGui::MenuItem("Stretch Vertex 0 (+X)", nullptr, false, hasSelectedObject)) {
                if (sceneObjects.StretchObjectVertexX(selectedObjectIndex_, 0, 1.0f)) {
                    Detail::AddEditorLog(Detail::EditorLogType::Info, "Stretched vertex 0 by +1.0 on X.");
                } else {
                    Detail::AddEditorLog(Detail::EditorLogType::Error, "Vertex stretch failed.");
                }
            }

            if (ImGui::MenuItem("Create Ico Sphere")) {
                if (sceneObjects.CreateIcoSphere()) {
                    selectedObjectIndex_ = sceneObjects.GetObjectCount() - 1;
                    selected_ = EditorSelection::DynamicObject;
                    Detail::GetModelApplyTargetIndex() = selectedObjectIndex_;
                    Detail::AddEditorLog(Detail::EditorLogType::Info, "Created IcoSphere.");
                } else {
                    Detail::AddEditorLog(Detail::EditorLogType::Error, "IcoSphere creation failed.");
                }
            }

            if (ImGui::MenuItem("Add Model File Name", nullptr, false, hasSelectedObject)) {
                if (sceneObjects.GetObjectExportFileName(selectedObjectIndex_).empty()) {
                    const std::string& path = sceneObjects.GetObjectModelPath(selectedObjectIndex_);
                    const size_t slash = path.find_last_of("/\\");
                    sceneObjects.SetObjectExportFileName(
                        selectedObjectIndex_,
                        slash == std::string::npos ? path : path.substr(slash + 1)
                    );
                }
            }

            if (ImGui::MenuItem("Add Box Collider", nullptr, false, hasSelectedObject)) {
                sceneObjects.AddBoxCollider(selectedObjectIndex_);
            }

            ImGui::Separator();
            ImGui::MenuItem("Show Colliders", nullptr, &showColliders_);

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
            Detail::AddEditorLog(Detail::EditorLogType::Info, "Play Button pressed.");
        }

        ImGui::SameLine();

        if (ImGui::Button("Emit")) {
            hitEffect.Emit({ 0.0f, 3.0f, 0.0f });
            Detail::AddEditorLog(Detail::EditorLogType::Info, "Effect emitted from menu bar.");
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
            Detail::GetModelApplyTargetIndex() = 0;
        }

        if (ImGui::Selectable(
            "Axis +X",
            selected_ == EditorSelection::AxisPositive
        )) {
            selected_ = EditorSelection::AxisPositive;
            selectedObjectIndex_ = 1;
            Detail::GetModelApplyTargetIndex() = 1;
        }

        if (ImGui::Selectable(
            "Axis -X",
            selected_ == EditorSelection::AxisNegative
        )) {
            selected_ = EditorSelection::AxisNegative;
            selectedObjectIndex_ = 2;
            Detail::GetModelApplyTargetIndex() = 2;
        }

        ImGui::Separator();

        if (sceneObjects.GetObjectCount() > 3) {
            ImGui::TextDisabled("Created Objects");

            std::function<void(size_t)> drawObjectNode;
            drawObjectNode = [&](size_t index) {
                bool hasChildren = false;
                for (size_t child = 3; child < sceneObjects.GetObjectCount(); ++child) {
                    if (sceneObjects.GetObjectParent(child) == index) {
                        hasChildren = true;
                        break;
                    }
                }

                ImGuiTreeNodeFlags flags =
                    ImGuiTreeNodeFlags_OpenOnArrow |
                    ImGuiTreeNodeFlags_SpanAvailWidth;
                if (!hasChildren) {
                    flags |= ImGuiTreeNodeFlags_Leaf |
                        ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }
                if (selected_ == EditorSelection::DynamicObject &&
                    selectedObjectIndex_ == index) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                ImGui::PushID(static_cast<int>(index));
                const bool open = ImGui::TreeNodeEx(
                    sceneObjects.GetObjectName(index).c_str(),
                    flags
                );
                if (ImGui::IsItemClicked()) {
                    selected_ = EditorSelection::DynamicObject;
                    selectedObjectIndex_ = index;
                    Detail::GetModelApplyTargetIndex() = index;
                }
                if (open && hasChildren) {
                    for (size_t child = 3; child < sceneObjects.GetObjectCount(); ++child) {
                        if (sceneObjects.GetObjectParent(child) == index) {
                            drawObjectNode(child);
                        }
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            };

            for (size_t index = 3; index < sceneObjects.GetObjectCount(); ++index) {
                const size_t parent = sceneObjects.GetObjectParent(index);
                if (parent == SceneObjectController::kNoParent || parent < 3) {
                    drawObjectNode(index);
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

        Detail::DrawTransformInspector(object);
        Detail::DrawMaterialInspector(object);

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

        Detail::DrawTransformInspector(object);
        Detail::DrawMaterialInspector(object);

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

        Detail::DrawTransformInspector(object);
        Detail::DrawMaterialInspector(object);

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

        Detail::DrawTransformInspector(object);
        Detail::DrawMaterialInspector(object);

        if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("Source: %s", sceneObjects.GetObjectModelPath(selectedObjectIndex_).c_str());

            char fileName[260]{};
            strncpy_s(
                fileName,
                sizeof(fileName),
                sceneObjects.GetObjectExportFileName(selectedObjectIndex_).c_str(),
                _TRUNCATE
            );
            if (ImGui::InputText("Export File Name", fileName, sizeof(fileName))) {
                sceneObjects.SetObjectExportFileName(selectedObjectIndex_, fileName);
            }
        }

        if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (SceneObjectController::BoxCollider* collider =
                sceneObjects.GetBoxCollider(selectedObjectIndex_)) {
                ImGui::TextDisabled("Type: BOX");
                if (sceneObjects.IsObjectColliding(selectedObjectIndex_)) {
                    ImGui::TextColored(
                        ImVec4(1.0f, 0.25f, 0.2f, 1.0f),
                        "Collision: HIT"
                    );
                } else {
                    ImGui::TextDisabled("Collision: None");
                }
                ImGui::DragFloat3("Center", &collider->center.x, 0.05f);
                if (ImGui::DragFloat3("Size", &collider->size.x, 0.05f, 0.01f, 1000.0f)) {
                    collider->size.x = (std::max)(collider->size.x, 0.01f);
                    collider->size.y = (std::max)(collider->size.y, 0.01f);
                    collider->size.z = (std::max)(collider->size.z, 0.01f);
                }
                if (ImGui::Button("Remove Box Collider")) {
                    sceneObjects.RemoveBoxCollider(selectedObjectIndex_);
                }
            } else if (ImGui::Button("Add Box Collider")) {
                sceneObjects.AddBoxCollider(selectedObjectIndex_);
            }
        }

        break;
    }

    default:
        ImGui::TextDisabled("No inspector available.");
        break;

    case EditorSelection::Asset:
    {
        size_t createdIndex = static_cast<size_t>(-1);

        if (Detail::DrawAssetInspector(
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
        if (Detail::DrawAssetItem(label, path)) {
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
    Detail::InitializeEditorConsoleOnce();

    static bool showInfo = true;
    static bool showWarning = true;
    static bool showError = true;
    static bool autoScroll = true;

    ImGui::Begin("Console");

    if (ImGui::Button("Clear")) {
        Detail::GetEditorLogs().clear();
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

    for (const Detail::EditorConsoleLog& log : Detail::GetEditorLogs()) {
        if (log.type == Detail::EditorLogType::Info && !showInfo) {
            continue;
        }

        if (log.type == Detail::EditorLogType::Warning && !showWarning) {
            continue;
        }

        if (log.type == Detail::EditorLogType::Error && !showError) {
            continue;
        }

        ImGui::TextColored(
            Detail::GetEditorLogColor(log.type),
            "%s %s",
            Detail::GetEditorLogPrefix(log.type),
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
            animationDebugPanel_.Draw(
                animationDebug
            );

            ImGui::Separator();

            // 既存のJoint・再生速度などの編集画面
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

