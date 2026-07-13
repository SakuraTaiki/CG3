#include "CameraDebugController.h"

#include "Camera.h"

#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif


namespace {
    constexpr float kPi =
        3.14159265358979323846f;

    constexpr float kPitchLimit =
        kPi * 0.5f - 0.01f;
}


void CameraDebugController::InitializeFromCamera(
    Camera* camera
) {
    if (!camera) {
        return;
    }

    const Vector3& rotate =
        camera->GetRotate();

    yaw_ = rotate.y;
    pitch_ = rotate.x;

    Vector3 right;
    Vector3 up;
    Vector3 forward;

    CalculateCameraAxes(
        right,
        up,
        forward
    );

    const Vector3& position =
        camera->GetTranslate();

    distance_ = 10.0f;
    targetDistance_ = distance_;

    focusPoint_ = {
        position.x + forward.x * distance_,
        position.y + forward.y * distance_,
        position.z + forward.z * distance_
    };

    initialized_ = true;
}

void CameraDebugController::CalculateCameraAxes(
    Vector3& right,
    Vector3& up,
    Vector3& forward
) const {
    const Vector3 scale = {
        1.0f,
        1.0f,
        1.0f
    };

    const Vector3 rotate = {
        pitch_,
        yaw_,
        0.0f
    };

    const Vector3 translate = {
        0.0f,
        0.0f,
        0.0f
    };

    const Matrix4x4 rotationMatrix =
        Math::MakeAffineMatrix(
            scale,
            rotate,
            translate
        );

    right = {
        rotationMatrix.m[0][0],
        rotationMatrix.m[0][1],
        rotationMatrix.m[0][2]
    };

    up = {
        rotationMatrix.m[1][0],
        rotationMatrix.m[1][1],
        rotationMatrix.m[1][2]
    };

    forward = {
        rotationMatrix.m[2][0],
        rotationMatrix.m[2][1],
        rotationMatrix.m[2][2]
    };

    right = Math::Normalize(right);
    up = Math::Normalize(up);
    forward = Math::Normalize(forward);
}


void CameraDebugController::Update(
    Camera* camera
) {
#ifdef USE_IMGUI
    if (!camera) {
        return;
    }

    if (!initialized_) {
        InitializeFromCamera(camera);
    }

    ImGuiIO& io =
        ImGui::GetIO();

    const float deltaTime =
        std::clamp(
            io.DeltaTime,
            0.0f,
            0.1f
        );

    // =========================================
    // Orbit
    // Alt + 左ドラッグ、または右ドラッグ
    // =========================================

    const bool orbitWithLeft =
        io.KeyAlt &&
        ImGui::IsMouseDown(
            ImGuiMouseButton_Left
        );

    const bool orbitWithRight =
        ImGui::IsMouseDown(
            ImGuiMouseButton_Right
        );

    if (orbitWithLeft || orbitWithRight) {
        yaw_ +=
            io.MouseDelta.x *
            rotateSensitivity_;

        pitch_ +=
            io.MouseDelta.y *
            rotateSensitivity_;

        pitch_ = std::clamp(
            pitch_,
            -kPitchLimit,
            kPitchLimit
        );
    }

    Vector3 right;
    Vector3 up;
    Vector3 forward;

    CalculateCameraAxes(
        right,
        up,
        forward
    );

    // =========================================
    // Pan
    // 中ドラッグ
    // =========================================

    if (ImGui::IsMouseDown(
        ImGuiMouseButton_Middle
    )) {
        // 遠いほど大きく移動する
        const float panSpeed =
            std::max(
                distance_,
                1.0f
            ) * panSensitivity_;

        const float horizontal =
            io.MouseDelta.x *
            panSpeed;

        const float vertical =
            -io.MouseDelta.y *
            panSpeed;

        focusPoint_.x +=
            right.x * horizontal +
            up.x * vertical;

        focusPoint_.y +=
            right.y * horizontal +
            up.y * vertical;

        focusPoint_.z +=
            right.z * horizontal +
            up.z * vertical;
    }

    // =========================================
    // Smooth Zoom
    // =========================================

    if (io.MouseWheel != 0.0f) {
        // 距離に対して割合でズームする
        // 遠距離では大きく、近距離では細かく動く
        targetDistance_ *=
            std::exp(
                -io.MouseWheel *
                zoomSensitivity_
            );

        targetDistance_ = std::clamp(
            targetDistance_,
            minimumDistance_,
            maximumDistance_
        );
    }

    // フレームレートに依存しにくい補間
    const float zoomBlend =
        1.0f -
        std::exp(
            -zoomSmoothness_ *
            deltaTime
        );

    distance_ +=
        (targetDistance_ - distance_) *
        zoomBlend;

    distance_ = std::clamp(
        distance_,
        minimumDistance_,
        maximumDistance_
    );

    // =========================================
    // Cameraへ反映
    // =========================================

    Vector3 cameraPosition = {
        focusPoint_.x -
            forward.x * distance_,

        focusPoint_.y -
            forward.y * distance_,

        focusPoint_.z -
            forward.z * distance_
    };

    camera->SetTranslate(
        cameraPosition
    );

    camera->SetRotate({
        pitch_,
        yaw_,
        0.0f
        });
#else
    (void)camera;
#endif
}