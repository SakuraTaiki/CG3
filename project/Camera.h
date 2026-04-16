#pragma once
#include "MyMath.h"

struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
};

class Camera
{
public:
    void Update();


private:
    Transform transform_ = { {1,1,1}, {0,0,0}, {0,0,0} };
    Matrix4x4 viewMatrix_;
    Matrix4x4 projectionMatrix_;

    TransformationMatrix* transformationData_ = nullptr;

};

