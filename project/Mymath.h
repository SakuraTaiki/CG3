#pragma once
#include<cmath>

class MyMath {
public:
        struct Vector2 {
            float x = 0.0f;
            float y = 0.0f;

            Vector2() = default;
            Vector2(float x, float y) : x(x), y(y) {}

            Vector2 operator+(const Vector2& v) const;
            Vector2 operator-(const Vector2& v) const;
            Vector2 operator*(float s) const;

            float Length() const;
            Vector2 Normalize() const;
        };

        struct Vector3 {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;

            Vector3() = default;
            Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

            Vector3 operator+(const Vector3& v) const;
            Vector3 operator-(const Vector3& v) const;
            Vector3 operator*(float s) const;

            float Length() const;
            Vector3 Normalize() const;

            static float Dot(const Vector3& a, const Vector3& b);
            static Vector3 Cross(const Vector3& a, const Vector3& b);
        };

        struct Vector4 {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            float w = 0.0f;

            Vector4() = default;
            Vector4(float x, float y, float z, float w)
                : x(x), y(y), z(z), w(w) {
            }
        };

        struct Matrix4x4 {
            float m[4][4]{};

            static Matrix4x4 Identity();
            static Matrix4x4 Scale(const Vector3& s);
            static Matrix4x4 RotateX(float rad);
            static Matrix4x4 RotateY(float rad);
            static Matrix4x4 RotateZ(float rad);
            static Matrix4x4 Translate(const Vector3& t);

            static Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b);
        };

        // ==========================
        // Transform
        // ==========================
        struct Transform {
            Vector3 scale{ 1.0f, 1.0f, 1.0f };
            Vector3 rotate{ 0.0f, 0.0f, 0.0f };   // ラジアン
            Vector3 translate{ 0.0f, 0.0f, 0.0f };

            Matrix4x4 ToMatrix() const;
        };
};