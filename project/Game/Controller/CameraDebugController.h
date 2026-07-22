#pragma once

#include "MyMath.h"

class Camera;
class Input;

class CameraDebugController {
public:
    void Update(Camera* camera, Input* input);

private:
    void InitializeFromCamera(Camera* camera);

    void CalculateCameraAxes(
        Vector3& right,
        Vector3& up,
        Vector3& forward
    ) const;


private:

    bool initialized_ = false;

    // カメラが注視する中心
    Vector3 focusPoint_ = {
        0.0f,
        0.0f,
        0.0f
    };

    // 回転
    float yaw_ = 0.0f;
    float pitch_ = 0.3f;

    // 現在距離と目標距離
    float distance_ = 10.0f;
    float targetDistance_ = 10.0f;

    // 操作感度
    float rotateSensitivity_ = 0.005f;
    float panSensitivity_ = 0.002f;
    float zoomSensitivity_ = 0.15f;

    // 大きいほど素早く目標距離へ近づく
    float zoomSmoothness_ = 12.0f;

    float minimumDistance_ = 0.1f;
    float maximumDistance_ = 500.0f;
};
