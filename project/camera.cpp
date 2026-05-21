#include "Camera.h"
#include "WinApp.h"

Camera::Camera()
    : transform_({ {1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 5.0f, -10.0f} })
    , worldMatrix_(Math::MakeIdentity4x4())
    , viewMatrix_(Math::MakeIdentity4x4())
    , projectionMatrix_(Math::MakeIdentity4x4())
    , viewProjectionMatrix_(Math::MakeIdentity4x4())
    , fovY_(0.45f)
    , aspectRatio_(float(WinApp::kClientWidth) / float(WinApp::kClientHeight))
    , nearClip_(0.1f)
    , farClip_(100.0f) {
}

void Camera::Update() {
    worldMatrix_ = Math::MakeAffineMatrix(
        transform_.scale,
        transform_.rotate,
        transform_.translate);

    viewMatrix_ = Math::Inverse(worldMatrix_);

    projectionMatrix_ = Math::MakePerspectiveFovMatrix(
        fovY_,
        aspectRatio_,
        nearClip_,
        farClip_);

    viewProjectionMatrix_ = Math::Multiply(viewMatrix_, projectionMatrix_);
}
