#pragma once

class Camera;

class CameraDebugController {
public:
    void Update(Camera* camera);

private:
    float wheelZoomSpeed_ = 2.0f;
};