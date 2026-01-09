#include "MyMath.h"
#include <cmath>

// ===== Vector2 =====
MyMath::Vector2 MyMath::Vector2::operator+(const Vector2& v) const {
    return { x + v.x, y + v.y };
}

MyMath::Vector2 MyMath::Vector2::operator-(const Vector2& v) const {
    return { x - v.x, y - v.y };
}

MyMath::Vector2 MyMath::Vector2::operator*(float s) const {
    return { x * s, y * s };
}

float MyMath::Vector2::Length() const {
    return std::sqrt(x * x + y * y);
}

MyMath::Vector2 MyMath::Vector2::Normalize() const {
    float len = Length();
    if (len == 0.0f) return {};
    return { x / len, y / len };
}

// ===== Vector3 =====
MyMath::Vector3 MyMath::Vector3::operator+(const Vector3& v) const {
    return { x + v.x, y + v.y, z + v.z };
}

MyMath::Vector3 MyMath::Vector3::operator-(const Vector3& v) const {
    return { x - v.x, y - v.y, z - v.z };
}

MyMath::Vector3 MyMath::Vector3::operator*(float s) const {
    return { x * s, y * s, z * s };
}

float MyMath::Vector3::Length() const {
    return std::sqrt(x * x + y * y + z * z);
}

MyMath::Vector3 MyMath::Vector3::Normalize() const {
    float len = Length();
    if (len == 0.0f) return {};
    return { x / len, y / len, z / len };
}

float MyMath::Vector3::Dot(const Vector3& a, const Vector3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

MyMath::Vector3 MyMath::Vector3::Cross(const Vector3& a, const Vector3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

// ===== Matrix4x4 =====
MyMath::Matrix4x4 MyMath::Matrix4x4::Identity() {
    Matrix4x4 mat{};
    for (int i = 0; i < 4; i++) {
        mat.m[i][i] = 1.0f;
    }
    return mat;
}

MyMath::Matrix4x4 MyMath::Matrix4x4::Scale(const Vector3& s) {
    Matrix4x4 mat = Identity();
    mat.m[0][0] = s.x;
    mat.m[1][1] = s.y;
    mat.m[2][2] = s.z;
    return mat;
}

MyMath::Matrix4x4 MyMath::Matrix4x4::RotateX(float rad) {
    Matrix4x4 mat = Identity();
    float c = std::cos(rad);
    float s = std::sin(rad);
    mat.m[1][1] = c;
    mat.m[1][2] = s;
    mat.m[2][1] = -s;
    mat.m[2][2] = c;
    return mat;
}

MyMath::Matrix4x4 MyMath::Matrix4x4::RotateY(float rad) {
    Matrix4x4 mat = Identity();
    float c = std::cos(rad);
    float s = std::sin(rad);
    mat.m[0][0] = c;
    mat.m[0][2] = -s;
    mat.m[2][0] = s;
    mat.m[2][2] = c;
    return mat;
}

MyMath::Matrix4x4 MyMath::Matrix4x4::RotateZ(float rad) {
    Matrix4x4 mat = Identity();
    float c = std::cos(rad);
    float s = std::sin(rad);
    mat.m[0][0] = c;
    mat.m[0][1] = s;
    mat.m[1][0] = -s;
    mat.m[1][1] = c;
    return mat;
}

MyMath::Matrix4x4 MyMath::Matrix4x4::Translate(const Vector3& t) {
    Matrix4x4 mat = Identity();
    mat.m[3][0] = t.x;
    mat.m[3][1] = t.y;
    mat.m[3][2] = t.z;
    return mat;
}

MyMath::Matrix4x4 MyMath::Matrix4x4::Multiply(
    const Matrix4x4& a,
    const Matrix4x4& b
) {
    Matrix4x4 result{};
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            result.m[row][col] =
                a.m[row][0] * b.m[0][col] +
                a.m[row][1] * b.m[1][col] +
                a.m[row][2] * b.m[2][col] +
                a.m[row][3] * b.m[3][col];
        }
    }
    return result;
}

// ===== Transform =====
MyMath::Matrix4x4 MyMath::Transform::ToMatrix() const {
    Matrix4x4 s = Matrix4x4::Scale(scale);
    Matrix4x4 rx = Matrix4x4::RotateX(rotate.x);
    Matrix4x4 ry = Matrix4x4::RotateY(rotate.y);
    Matrix4x4 rz = Matrix4x4::RotateZ(rotate.z);
    Matrix4x4 t = Matrix4x4::Translate(translate);

    Matrix4x4 r = Matrix4x4::Multiply(rx, ry);
    r = Matrix4x4::Multiply(r, rz);

    Matrix4x4 world = Matrix4x4::Multiply(s, r);
    world = Matrix4x4::Multiply(world, t);

    return world;
}