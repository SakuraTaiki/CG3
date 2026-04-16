#include "Camera.h"


void Camera::Update() {
    Matrix4x4 worldMatrix = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 viewMatrix = Math::Inverse(worldMatrix);
    

    transformationData_->WVP = wvpMatrix;
    transformationData_->World = worldMatrix;
}