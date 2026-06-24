#include "Camera.h"

Camera::Camera()
    : transform_({
        { 1.0f, 1.0f, 1.0f },
        { 0.3f, 0.0f, 0.0f },
        { 0.0f, 5.0f, -10.0f }
        })
    , worldMatrix_(Math::MakeIdentity4x4())
    , viewMatrix_(Math::MakeIdentity4x4())
    , projectionMatrix_(Math::MakeIdentity4x4())
    , viewProjectionMatrix_(Math::MakeIdentity4x4())
    , fovY_(0.45f)
    , aspectRatio_(16.0f / 9.0f)
    , nearClip_(0.1f)
    , farClip_(100.0f) {}

void Camera::Update() {
    // Camera の WorldMatrix を作る。
    worldMatrix_ = Math::MakeAffineMatrix(
        transform_.scale,
        transform_.rotate,
        transform_.translate
    );

    // Camera の WorldMatrix の逆行列が ViewMatrix。
    viewMatrix_ = Math::Inverse(worldMatrix_);

    // ProjectionMatrix を作る。
    projectionMatrix_ = Math::MakePerspectiveFovMatrix(
        fovY_,
        aspectRatio_,
        nearClip_,
        farClip_
    );

    // 描画でよく使う ViewProjection を先に作っておく。
    viewProjectionMatrix_ = Math::Multiply(viewMatrix_, projectionMatrix_);
}