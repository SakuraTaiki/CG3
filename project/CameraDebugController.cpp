#include "CameraDebugController.h"

#include "camera.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void CameraDebugController::DrawImGui(Camera* camera) {
#ifdef USE_IMGUI
    if (!camera) {
        return;
    }

    Vector3 rotate = camera->GetRotate();
    Vector3 translate = camera->GetTranslate();

    float fov = camera->GetFovY();
    float nearClip = camera->GetNearClip();
    float farClip = camera->GetFarClip();

    ImGui::SetNextWindowPos(
        ImVec2(460.0f, 20.0f),
        ImGuiCond_FirstUseEver
    );

    ImGui::SetNextWindowSize(
        ImVec2(360.0f, 360.0f),
        ImGuiCond_FirstUseEver
    );

    ImGui::Begin("Camera Debug Panel");

    ImGui::Text("Camera Transform");
    ImGui::Separator();

    if (ImGui::BeginTabBar("CameraTabs")) {
        if (ImGui::BeginTabItem("Transform")) {
            ImGui::Text("Position");
            ImGui::DragFloat3("Translate", &translate.x, 0.1f);

            ImGui::Spacing();

            ImGui::Text("Rotation");
            ImGui::DragFloat3("Rotate", &rotate.x, 0.01f);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Lens")) {
            ImGui::Text("Projection");
            ImGui::DragFloat("Fov", &fov, 0.01f, 0.1f, 2.0f);
            ImGui::DragFloat("Near Clip", &nearClip, 0.01f, 0.01f, 10.0f);
            ImGui::DragFloat("Far Clip", &farClip, 1.0f, 10.0f, 10000.0f);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Preset")) {
            if (ImGui::Button("Default Camera", ImVec2(220.0f, 32.0f))) {
                translate = { 0.0f, 5.0f, -10.0f };
                rotate = { 0.3f, 0.0f, 0.0f };
                fov = 0.45f;
                nearClip = 0.1f;
                farClip = 100.0f;
            }

            if (ImGui::Button("Terrain Check Camera", ImVec2(220.0f, 32.0f))) {
                translate = { 0.0f, 10.0f, -30.0f };
                rotate = { 0.35f, 0.0f, 0.0f };
                fov = 0.45f;
                nearClip = 0.1f;
                farClip = 500.0f;
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Separator();
    ImGui::Text("Current");
    ImGui::BulletText("Pos  : %.2f, %.2f, %.2f", translate.x, translate.y, translate.z);
    ImGui::BulletText("Rot  : %.2f, %.2f, %.2f", rotate.x, rotate.y, rotate.z);
    ImGui::BulletText("Fov  : %.2f", fov);

    camera->SetTranslate(translate);
    camera->SetRotate(rotate);
    camera->SetFovY(fov);
    camera->SetNearClip(nearClip);
    camera->SetFarClip(farClip);

    ImGui::End();
#endif
}