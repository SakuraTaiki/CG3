#include "CameraDebugController.h"

#include "Camera.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void CameraDebugController::Update(Camera* camera) {
#ifdef USE_IMGUI
    if (!camera) {
        return;
    }

    ImGuiIO& io =
        ImGui::GetIO();

    const float wheel =
        io.MouseWheel;

    if (wheel == 0.0f) {
        return;
    }

    Vector3 translate =
        camera->GetTranslate();

    const Matrix4x4& cameraWorld =
        camera->GetWorldMatrix();

    // カメラのローカル前方向（+Z）
    Vector3 forward = {
        cameraWorld.m[2][0],
        cameraWorld.m[2][1],
        cameraWorld.m[2][2]
    };

    forward =
        Math::Normalize(forward);

    const float movement =
        wheel * wheelZoomSpeed_;

    translate.x +=
        forward.x * movement;

    translate.y +=
        forward.y * movement;

    translate.z +=
        forward.z * movement;

    camera->SetTranslate(translate);
#else
    (void)camera;
#endif
}