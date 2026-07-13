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


