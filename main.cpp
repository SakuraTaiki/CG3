#include <windows.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <chrono>
#include <format>
#include <fstream>
#include <d3d12.h>
#include <dxgi1_6.h>
#include<cassert>
#include <dbghelp.h>
#include <strsafe.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include<vector>
#include "externals/imgui/imgui.h"
#include"externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/DirectXTex/DirectXTex.h"
#include"externals/DirectXTex/d3dx12.h"
#include <numbers>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

struct Vector2
{
	float x;
	float y;
};

struct Vector3
{
	float x;
	float y;
	float z;
};


struct Vector4
{
	float x;
	float y;
	float z;
	float w;
};

struct Matrix4x4
{
	float m[4][4];
};

struct Transform
{
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct VertexData
{
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct Material
{
	Vector4 color;
	int32_t enableLighting;
};

struct TransformationMatrix
{
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct DirectionalLight
{
	Vector4 color;//ライトの色
	Vector3 direction;//ライトの向き
	float intensity;//輝度
};

// 単位行列の作成
Matrix4x4 makeIdentity4x4()
{
	Matrix4x4 result = {};
	result.m[0][0] = 1.0f;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;

	result.m[1][0] = 0.0f;
	result.m[1][1] = 1.0f;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;

	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = 1.0f;
	result.m[2][3] = 0.0f;

	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = 0.0f;
	result.m[3][3] = 1.0f;
	return result;
};

//拡縮行列
Matrix4x4 MakeScaleMatrix(const Vector3& scale) {

	Matrix4x4 result{ scale.x, 0.0f, 0.0f, 0.0f, 0.0f, scale.y, 0.0f, 0.0f, 0.0f, 0.0f, scale.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	return result;
}

//回転行列
Matrix4x4 MakeRotateXMatrix(float theta) {
	float sin = std::sin(theta);
	float cos = std::cos(theta);

	Matrix4x4 result{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, cos, sin, 0.0f, 0.0f, -sin, cos, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	return result;
}

Matrix4x4 MakeRotateYMatrix(float theta) {
	float sin = std::sin(theta);
	float cos = std::cos(theta);

	Matrix4x4 result{ cos, 0.0f, -sin, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, sin, 0.0f, cos, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	return result;
}

Matrix4x4 MakeRotateZMatrix(float theta) {
	float sin = std::sin(theta);
	float cos = std::cos(theta);

	Matrix4x4 result{ cos, sin, 0.0f, 0.0f, -sin, cos, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	return result;
}

Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, translate.x, translate.y, translate.z, 1.0f };

	return result;
}



Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result = {};
	result.m[0][0] = (m1.m[0][0] * m2.m[0][0]) + (m1.m[0][1] * m2.m[1][0]) + (m1.m[0][2] * m2.m[2][0]) + (m1.m[0][3] * m2.m[3][0]);
	result.m[0][1] = (m1.m[0][0] * m2.m[0][1]) + (m1.m[0][1] * m2.m[1][1]) + (m1.m[0][2] * m2.m[2][1]) + (m1.m[0][3] * m2.m[3][1]);
	result.m[0][2] = (m1.m[0][0] * m2.m[0][2]) + (m1.m[0][1] * m2.m[1][2]) + (m1.m[0][2] * m2.m[2][2]) + (m1.m[0][3] * m2.m[3][2]);
	result.m[0][3] = (m1.m[0][0] * m2.m[0][3]) + (m1.m[0][1] * m2.m[1][3]) + (m1.m[0][2] * m2.m[2][3]) + (m1.m[0][3] * m2.m[3][3]);

	result.m[1][0] = (m1.m[1][0] * m2.m[0][0]) + (m1.m[1][1] * m2.m[1][0]) + (m1.m[1][2] * m2.m[2][0]) + (m1.m[1][3] * m2.m[3][0]);
	result.m[1][1] = (m1.m[1][0] * m2.m[0][1]) + (m1.m[1][1] * m2.m[1][1]) + (m1.m[1][2] * m2.m[2][1]) + (m1.m[1][3] * m2.m[3][1]);
	result.m[1][2] = (m1.m[1][0] * m2.m[0][2]) + (m1.m[1][1] * m2.m[1][2]) + (m1.m[1][2] * m2.m[2][2]) + (m1.m[1][3] * m2.m[3][2]);
	result.m[1][3] = (m1.m[1][0] * m2.m[0][3]) + (m1.m[1][1] * m2.m[1][3]) + (m1.m[1][2] * m2.m[2][3]) + (m1.m[1][3] * m2.m[3][3]);

	result.m[2][0] = (m1.m[2][0] * m2.m[0][0]) + (m1.m[2][1] * m2.m[1][0]) + (m1.m[2][2] * m2.m[2][0]) + (m1.m[2][3] * m2.m[3][0]);
	result.m[2][1] = (m1.m[2][0] * m2.m[0][1]) + (m1.m[2][1] * m2.m[1][1]) + (m1.m[2][2] * m2.m[2][1]) + (m1.m[2][3] * m2.m[3][1]);
	result.m[2][2] = (m1.m[2][0] * m2.m[0][2]) + (m1.m[2][1] * m2.m[1][2]) + (m1.m[2][2] * m2.m[2][2]) + (m1.m[2][3] * m2.m[3][2]);
	result.m[2][3] = (m1.m[2][0] * m2.m[0][3]) + (m1.m[2][1] * m2.m[1][3]) + (m1.m[2][2] * m2.m[2][3]) + (m1.m[2][3] * m2.m[3][3]);

	result.m[3][0] = (m1.m[3][0] * m2.m[0][0]) + (m1.m[3][1] * m2.m[1][0]) + (m1.m[3][2] * m2.m[2][0]) + (m1.m[3][3] * m2.m[3][0]);
	result.m[3][1] = (m1.m[3][0] * m2.m[0][1]) + (m1.m[3][1] * m2.m[1][1]) + (m1.m[3][2] * m2.m[2][1]) + (m1.m[3][3] * m2.m[3][1]);
	result.m[3][2] = (m1.m[3][0] * m2.m[0][2]) + (m1.m[3][1] * m2.m[1][2]) + (m1.m[3][2] * m2.m[2][2]) + (m1.m[3][3] * m2.m[3][2]);
	result.m[3][3] = (m1.m[3][0] * m2.m[0][3]) + (m1.m[3][1] * m2.m[1][3]) + (m1.m[3][2] * m2.m[2][3]) + (m1.m[3][3] * m2.m[3][3]);
	return result;
}

Matrix4x4 Inverse(const Matrix4x4& m) {
	Matrix4x4 result{};
	float determinant =
		m.m[0][3] * m.m[1][2] * m.m[2][1] * m.m[3][0] - m.m[0][2] * m.m[1][3] * m.m[2][1] * m.m[3][0] - m.m[0][3] * m.m[1][1] * m.m[2][2] * m.m[3][0] + m.m[0][1] * m.m[1][3] * m.m[2][2] * m.m[3][0] +
		m.m[0][2] * m.m[1][1] * m.m[2][3] * m.m[3][0] - m.m[0][1] * m.m[1][2] * m.m[2][3] * m.m[3][0] - m.m[0][3] * m.m[1][2] * m.m[2][0] * m.m[3][1] + m.m[0][2] * m.m[1][3] * m.m[2][0] * m.m[3][1] +
		m.m[0][3] * m.m[1][0] * m.m[2][2] * m.m[3][1] - m.m[0][0] * m.m[1][3] * m.m[2][2] * m.m[3][1] - m.m[0][2] * m.m[1][0] * m.m[2][3] * m.m[3][1] + m.m[0][0] * m.m[1][2] * m.m[2][3] * m.m[3][1] +
		m.m[0][3] * m.m[1][1] * m.m[2][0] * m.m[3][2] - m.m[0][1] * m.m[1][3] * m.m[2][0] * m.m[3][2] - m.m[0][3] * m.m[1][0] * m.m[2][1] * m.m[3][2] + m.m[0][0] * m.m[1][3] * m.m[2][1] * m.m[3][2] +
		m.m[0][1] * m.m[1][0] * m.m[2][3] * m.m[3][2] - m.m[0][0] * m.m[1][1] * m.m[2][3] * m.m[3][2] - m.m[0][2] * m.m[1][1] * m.m[2][0] * m.m[3][3] + m.m[0][1] * m.m[1][2] * m.m[2][0] * m.m[3][3] +
		m.m[0][2] * m.m[1][0] * m.m[2][1] * m.m[3][3] - m.m[0][0] * m.m[1][2] * m.m[2][1] * m.m[3][3] - m.m[0][1] * m.m[1][0] * m.m[2][2] * m.m[3][3] + m.m[0][0] * m.m[1][1] * m.m[2][2] * m.m[3][3];


	float determinantRecp = 1.0f / determinant;
	result.m[0][0] = (m.m[1][2] * m.m[2][3] * m.m[3][1] - m.m[1][3] * m.m[2][2] * m.m[3][1] + m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][3] * m.m[3][2] -
		m.m[1][2] * m.m[2][1] * m.m[3][3] + m.m[1][1] * m.m[2][2] * m.m[3][3]) *
		determinantRecp;
	result.m[0][1] = (m.m[0][3] * m.m[2][2] * m.m[3][1] - m.m[0][2] * m.m[2][3] * m.m[3][1] - m.m[0][3] * m.m[2][1] * m.m[3][2] + m.m[0][1] * m.m[2][3] * m.m[3][2] +
		m.m[0][2] * m.m[2][1] * m.m[3][3] - m.m[0][1] * m.m[2][2] * m.m[3][3]) *
		determinantRecp;
	result.m[0][2] = (m.m[0][2] * m.m[1][3] * m.m[3][1] - m.m[0][3] * m.m[1][2] * m.m[3][1] + m.m[0][3] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][3] * m.m[3][2] -
		m.m[0][2] * m.m[1][1] * m.m[3][3] + m.m[0][1] * m.m[1][2] * m.m[3][3]) *
		determinantRecp;
	result.m[0][3] = (m.m[0][3] * m.m[1][2] * m.m[2][1] - m.m[0][2] * m.m[1][3] * m.m[2][1] - m.m[0][3] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][3] * m.m[2][2] +
		m.m[0][2] * m.m[1][1] * m.m[2][3] - m.m[0][1] * m.m[1][2] * m.m[2][3]) *
		determinantRecp;
	result.m[1][0] = (m.m[1][3] * m.m[2][2] * m.m[3][0] - m.m[1][2] * m.m[2][3] * m.m[3][0] - m.m[1][3] * m.m[2][0] * m.m[3][2] + m.m[1][0] * m.m[2][3] * m.m[3][2] +
		m.m[1][2] * m.m[2][0] * m.m[3][3] - m.m[1][0] * m.m[2][2] * m.m[3][3]) *
		determinantRecp;
	result.m[1][1] = (m.m[0][2] * m.m[2][3] * m.m[3][0] - m.m[0][3] * m.m[2][2] * m.m[3][0] + m.m[0][3] * m.m[2][0] * m.m[3][2] - m.m[0][0] * m.m[2][3] * m.m[3][2] -
		m.m[0][2] * m.m[2][0] * m.m[3][3] + m.m[0][0] * m.m[2][2] * m.m[3][3]) *
		determinantRecp;
	result.m[1][2] = (m.m[0][3] * m.m[1][2] * m.m[3][0] - m.m[0][2] * m.m[1][3] * m.m[3][0] - m.m[0][3] * m.m[1][0] * m.m[3][2] + m.m[0][0] * m.m[1][3] * m.m[3][2] +
		m.m[0][2] * m.m[1][0] * m.m[3][3] - m.m[0][0] * m.m[1][2] * m.m[3][3]) *
		determinantRecp;
	result.m[1][3] = (m.m[0][2] * m.m[1][3] * m.m[2][0] - m.m[0][3] * m.m[1][2] * m.m[2][0] + m.m[0][3] * m.m[1][0] * m.m[2][2] - m.m[0][0] * m.m[1][3] * m.m[2][2] -
		m.m[0][2] * m.m[1][0] * m.m[2][3] + m.m[0][0] * m.m[1][2] * m.m[2][3]) *
		determinantRecp;
	result.m[2][0] = (m.m[1][1] * m.m[2][3] * m.m[3][0] - m.m[1][3] * m.m[2][1] * m.m[3][0] + m.m[1][3] * m.m[2][0] * m.m[3][1] - m.m[1][0] * m.m[2][3] * m.m[3][1] -
		m.m[1][1] * m.m[2][0] * m.m[3][3] + m.m[1][0] * m.m[2][1] * m.m[3][3]) *
		determinantRecp;
	result.m[2][1] = (m.m[0][3] * m.m[2][1] * m.m[3][0] - m.m[0][1] * m.m[2][3] * m.m[3][0] - m.m[0][3] * m.m[2][0] * m.m[3][1] + m.m[0][0] * m.m[2][3] * m.m[3][1] +
		m.m[0][1] * m.m[2][0] * m.m[3][3] - m.m[0][0] * m.m[2][1] * m.m[3][3]) *
		determinantRecp;
	result.m[2][2] = (m.m[0][1] * m.m[1][3] * m.m[3][0] - m.m[0][3] * m.m[1][1] * m.m[3][0] + m.m[0][3] * m.m[1][0] * m.m[3][1] - m.m[0][0] * m.m[1][3] * m.m[3][1] -
		m.m[0][1] * m.m[1][0] * m.m[3][3] + m.m[0][0] * m.m[1][1] * m.m[3][3]) *
		determinantRecp;
	result.m[2][3] = (m.m[0][3] * m.m[1][1] * m.m[2][0] - m.m[0][1] * m.m[1][3] * m.m[2][0] - m.m[0][3] * m.m[1][0] * m.m[2][1] + m.m[0][0] * m.m[1][3] * m.m[2][1] +
		m.m[0][1] * m.m[1][0] * m.m[2][3] - m.m[0][0] * m.m[1][1] * m.m[2][3]) *
		determinantRecp;
	result.m[3][0] = (m.m[1][2] * m.m[2][1] * m.m[3][0] - m.m[1][1] * m.m[2][2] * m.m[3][0] - m.m[1][2] * m.m[2][0] * m.m[3][1] + m.m[1][0] * m.m[2][2] * m.m[3][1] +
		m.m[1][1] * m.m[2][0] * m.m[3][2] - m.m[1][0] * m.m[2][1] * m.m[3][2]) *
		determinantRecp;
	result.m[3][1] = (m.m[0][1] * m.m[2][2] * m.m[3][0] - m.m[0][2] * m.m[2][1] * m.m[3][0] + m.m[0][2] * m.m[2][0] * m.m[3][1] - m.m[0][0] * m.m[2][2] * m.m[3][1] -
		m.m[0][1] * m.m[2][0] * m.m[3][2] + m.m[0][0] * m.m[2][1] * m.m[3][2]) *
		determinantRecp;
	result.m[3][2] = (m.m[0][2] * m.m[1][1] * m.m[3][0] - m.m[0][1] * m.m[1][2] * m.m[3][0] - m.m[0][2] * m.m[1][0] * m.m[3][1] + m.m[0][0] * m.m[1][2] * m.m[3][1] +
		m.m[0][1] * m.m[1][0] * m.m[3][2] - m.m[0][0] * m.m[1][1] * m.m[3][2]) *
		determinantRecp;
	result.m[3][3] = (m.m[0][1] * m.m[1][2] * m.m[2][0] - m.m[0][2] * m.m[1][1] * m.m[2][0] + m.m[0][2] * m.m[1][0] * m.m[2][1] - m.m[0][0] * m.m[1][2] * m.m[2][1] -
		m.m[0][1] * m.m[1][0] * m.m[2][2] + m.m[0][0] * m.m[1][1] * m.m[2][2]) *
		determinantRecp;
	return result;
}

//透視投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip)
{
	Matrix4x4 result = {};
	float f = 1.0f / tanf(fovY / 2.0f);
	result.m[0][0] = f / aspectRatio;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;

	result.m[1][0] = 0.0f;
	result.m[1][1] = f;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;

	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = farClip / (farClip - nearClip);
	result.m[2][3] = 1.0f;

	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = -nearClip * farClip / (farClip - nearClip);
	result.m[3][3] = 0.0f;
	return result;
}


Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
	Matrix4x4 rotateXMatrix = MakeRotateXMatrix(rotate.x);
	Matrix4x4 rotateYMatrix = MakeRotateYMatrix(rotate.y);
	Matrix4x4 rotateZMatrix = MakeRotateZMatrix(rotate.z);
	Matrix4x4 rotateMatrix = Multiply(Multiply(rotateXMatrix, rotateYMatrix), rotateZMatrix);
	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

	Matrix4x4 worldMatrix = Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);

	return worldMatrix;
}

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
	Matrix4x4 result{};
	result.m[0][0] = 2 / (right - left);
	result.m[0][1] = 0;
	result.m[0][2] = 0;
	result.m[0][3] = 0;
	result.m[1][0] = 0;
	result.m[1][1] = 2 / (top - bottom);
	result.m[1][2] = 0;
	result.m[1][3] = 0;
	result.m[2][0] = 0;
	result.m[2][1] = 0;
	result.m[2][2] = 1 / (farClip - nearClip);
	result.m[2][3] = 0;
	result.m[3][0] = (left + right) / (left - right);
	result.m[3][1] = (top + bottom) / (bottom - top);
	result.m[3][2] = nearClip / (nearClip - farClip);
	result.m[3][3] = 1;


	return result;
}

Matrix4x4 MakepersfectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
	Matrix4x4 result{};
	result.m[0][0] = (1 / aspectRatio) * (1 / std::tan(fovY / 2));
	result.m[0][1] = 0;
	result.m[0][2] = 0;
	result.m[0][2] = 0;
	result.m[0][3] = 0;
	result.m[1][1] = 0;
	result.m[1][1] = (1 / std::tan(fovY / 2));
	result.m[1][2] = 0;
	result.m[1][3] = 0;
	result.m[2][0] = 0;
	result.m[2][1] = 0;
	result.m[2][2] = (farClip / (farClip - nearClip));
	result.m[2][3] = 1;
	result.m[3][0] = 0;
	result.m[3][1] = 0;
	result.m[3][2] = (-(nearClip * farClip) / (farClip - nearClip));
	result.m[3][3] = 0;
	return result;
}


static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception)
{
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./DUMPS", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);




	return EXCEPTION_EXECUTE_HANDLER;
}


std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

//resouces作成の関数
ID3D12Resource* createBufferResouces(ID3D12Device* device, size_t sizeInBytes)
{
	//頂点とリソース用のヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; //3_0Exでの変更点

	//頂点リソースの設定
	D3D12_RESOURCE_DESC vertResoucesDesc{};

	//バッファーリソーステクスチャの場合は別の指定をする
	vertResoucesDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertResoucesDesc.Width = sizeInBytes * 3;

	//バッファの場合は1にする
	vertResoucesDesc.Height = 1;
	vertResoucesDesc.DepthOrArraySize = 1;
	vertResoucesDesc.MipLevels = 1;
	vertResoucesDesc.SampleDesc.Count = 1;

	//バッファの場合はこれをする決まり
	vertResoucesDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//実際に頂点リソースを作る
	ID3D12Resource* vertexResouces = nullptr;
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&vertResoucesDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResouces));
	assert(SUCCEEDED(hr));
	return vertexResouces;
}




void Log(std::ostream& os, const std::string& message)
{
	os << message << std::endl;
	OutputDebugStringA(message.c_str());
}

ID3D12DescriptorHeap* CreateDescriptorHeap(
	ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible
)
{
	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	return descriptorHeap;
}

void ShowSRTWindow(Transform& transform)
{

	ImGui::SliderFloat3("Scale", &transform.scale.x, 0.1f, 10.0f);
	ImGui::SliderFloat3("Rotate", &transform.rotate.x, -3.14159f, 3.14159f);
	ImGui::SliderFloat3("Translate", &transform.translate.x, -100.0f, 100.0f);

}






LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg,
	WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return true;
	}

	//メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
		//windowsが放棄された
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	//標準メッセージを行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

IDxcBlob* CompileShander(
	const std::wstring& filePath,
	const wchar_t* profile,
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxCompiler,
	IDxcIncludeHandler* includeHandler,
	std::ostream& os)
{
	//hlslファイルを読み込む
	Log(os, ConvertString(std::format(L"Begin CompileShader,path:{}, profile:{}\n", filePath, profile)));
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));

	DxcBuffer shaderSoucesBuffer;
	shaderSoucesBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSoucesBuffer.Size = shaderSource->GetBufferSize();
	shaderSoucesBuffer.Encoding = DXC_CP_UTF8;
	//complieする
	LPCWSTR argument[] = { filePath.c_str(),
	L"-E",L"main",
	L"-T", profile,
	L"-Zi", L"-Qembed_debug",
	L"-Od",
	L"-Zpr" };

	IDxcResult* shaderResult = nullptr;
	hr = dxCompiler->Compile(
		&shaderSoucesBuffer,
		argument,
		_countof(argument),
		includeHandler,
		IID_PPV_ARGS(&shaderResult));

	assert(SUCCEEDED(hr));
	//警告エラーが出てないか確認する
	IDxcBlobUtf8* shanderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shanderError), nullptr);
	if (shanderError != nullptr && shanderError->GetStringLength() != 0)
	{
		Log(os, shanderError->GetStringPointer());
		assert(false);

	}
	//Compile結果を受けて返す
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	Log(os, ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));
	shaderSource->Release();
	shaderResult->Release();
	return shaderBlob;

}

DirectX::ScratchImage LoadTexture(const std::string& filePath)
{
	//テクスチャファイルを読み込んでプログラムで扱えるようにする

	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	//ミニマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	//ミップマップのデータを返す
	return mipImages;
}

ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata)
{
	//metadataを軸にResoucesの設定
	D3D12_RESOURCE_DESC resouceDesc{};
	resouceDesc.Width = UINT(metadata.width);//textreの幅
	resouceDesc.Height = UINT(metadata.height);//textreの幅
	resouceDesc.MipLevels = UINT(metadata.mipLevels);//mipmapの数
	resouceDesc.DepthOrArraySize = UINT16(metadata.arraySize);//奥行き or 配列のtextreの配列数
	resouceDesc.Format = metadata.format;//textreのformat
	resouceDesc.SampleDesc.Count = 1;//サンプリングカウント1固定
	resouceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);//textreの次元数　普段使っているのは2次元

	//利用するheapの設定　非常に特殊な運用　02_04exで一般ケース版がある
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;//細かい設定お行う


	//resoucesの作成
	ID3D12Resource* resouce = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,//heapの設定
		D3D12_HEAP_FLAG_NONE,//heapの特殊設定
		&resouceDesc,//Resoucesの設定
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&resouce));//初回Resoucesstate textre破棄本読むだけ
	assert(SUCCEEDED(hr));
	return resouce;
}

ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height)
{
	//生成するResourceの設定
	D3D12_RESOURCE_DESC resouceDesc{};
	resouceDesc.Width = width;//Textureの幅
	resouceDesc.Height = height;//Textureの高さ
	resouceDesc.MipLevels = 1;//mipmapの数
	resouceDesc.DepthOrArraySize = 1;//奥行きor配列のTextureの配列
	resouceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//DepthStencilとして利用可能なフォーマット
	resouceDesc.SampleDesc.Count = 1;
	resouceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2次元
	resouceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//DepthStencilとして使う通知

	//利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;//VRAM上に作る

	//深度値のクリア 
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;//1.0f(最大値)でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//フォーマット　Resourceと合わせる


	//Resourceの生産
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,//Heapの設定
		D3D12_HEAP_FLAG_NONE, //Heapの特殊な設定
		&resouceDesc, //Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE, //深度値を書き込む状態にしておく
		&depthClearValue, //Clear最適値
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}


[[nodiscard]]
ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList)
{
	std::vector<D3D12_SUBRESOURCE_DATA>subresources;
	DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	ID3D12Resource* intermediateResource = createBufferResouces(device, intermediateSize);
	UpdateSubresources(commandList, texture, intermediateResource, 0, 0, UINT(subresources.size()), subresources.data());
	//Textureへの転送後は利用できるようにD3D12_RESOURCE_STATE_COPY_DEST_RESOURCE_STATE_GENERIC_READへResourceStateを変更する

	//新しく作ったBarrier
	D3D12_RESOURCE_BARRIER barrier{};

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;

	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;

}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}


//////全mipdataについて
////for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; mipLevel++)
////{
////	const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);
////	//textureに転送
////	HRESULT hr = texture->WriteToSubresource
////	(
////		UINT(mipLevel),//全領域へコピー
////		nullptr,
////		img->pixels,//元データアドレス
////		UINT(img->rowPitch),//1ラインサイズ
////		UINT(img->slicePitch)//1枚サイズ
////
////	);
////	assert(SUCCEEDED(hr));
//}


bool useMonsterBall = true;



int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {


	SetUnhandledExceptionFilter(ExportDump);

	//comの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);




	//ログのフォルダ作成
	std::filesystem::create_directory("logs");

	//現在時刻を取得
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	//ログファイルの名前にコンマはいらないので削って秒にする
	std::chrono::time_point < std::chrono::system_clock, std::chrono::seconds >
		nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);

	//日本時間に変換
	std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };

	//年月日時分秒の文字列の取得
	std::string dateStrings = std::format("{:%Y%m%d_%H%M%S}", localTime);

	//ファイル名
	std::string  logFilePath = std::format("logs/") + dateStrings + "log";
	std::ofstream logStrem(logFilePath);
	WNDCLASS wc{};

	//windowプロシージャ
	wc.lpfnWndProc = WindowProc;

	//windowクラス名
	wc.lpszClassName = L"CG2WindowClass";

	//インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);

	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	//windowクラスを登録する
	RegisterClass(&wc);

	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;
	RECT wrc = { 0, 0, kClientWidth, kClientHeight };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	//windowsの生成
	HWND hwnd = CreateWindow(
		wc.lpszClassName, // 利用するクラス名
		L"CG2", // タイトルバーの文字
		WS_OVERLAPPEDWINDOW, // よく見るウィンドウタイトル
		CW_USEDEFAULT, // 表示X座標
		CW_USEDEFAULT, // 表示Y座標
		wrc.right - wrc.left, // ウィンドウ横幅
		wrc.bottom - wrc.top, // ウィンドウ縦幅
		nullptr, // 親ウィンドウハンドル
		nullptr, // メニューハンドル
		wc.hInstance, // インスタンスハンドル
		nullptr); // オプション

	ShowWindow(hwnd, SW_SHOW);




#ifdef _DEBUG
	ID3D12Debug1* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(true);
	}




#endif // !_DEBUG


	//DXGIファクトリーの生成
	IDXGIFactory7* dxgiFactory = nullptr;

	//関数が成功したかどうかSUCCEEDマクロで判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(hr));

	//仕様するアダプタ用生成の変数。最初にnullptrを入れておく
	IDXGIAdapter4* useAsapter = nullptr;

	//良い順でアダプタを読む
	for (int i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAsapter)) !=
		DXGI_ERROR_NOT_FOUND; ++i)

	{//アダプタの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAsapter->GetDesc3(&adapterDesc);

		assert(SUCCEEDED(hr));//取得できないのは一大事
		//ソフトウェアアダプタでなければ採用
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			//採用したアダプタ情報をログに出力 wstringの方なので注意
			Log(logStrem, ConvertString(std::format(L"use Adapater:{}\n", adapterDesc.Description)));
			break;
		}

		useAsapter = nullptr;//ソフトウェアアダプタのばあいは見なかったことにする
	}

	//見つからなかったので起動できない
	assert(useAsapter != nullptr);

	ID3D12Device* device = nullptr;
	//機能レベルとログの出力
	D3D_FEATURE_LEVEL featrueLevels[] =
	{ D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0 };
	const char* featrueLevelStrings[] = { "12.1", "12.1", "12.0" };
	for (size_t i = 0; i < _countof(featrueLevels); ++i)
	{
		//採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAsapter, featrueLevels[i], IID_PPV_ARGS(&device));
		//指定した機能レベルでデバイスが生成できたか確認
		if (SUCCEEDED(hr))
		{
			//静瀬尾できたのでループを抜ける
			Log(logStrem, (std::format("FeatrueLevel", featrueLevelStrings[i])));

			break;
		}
	}
	assert(device != nullptr);
	Log(logStrem, "complate crate D3D12Device!!!\n");//初期化ログを出す

#ifdef _DEBUG

	ID3D12InfoQueue* infoqueue = nullptr;

	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoqueue))))
	{
		//やばいときにエラーで止まる
		infoqueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		//エラー時に止まる
		infoqueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		//緊急時に止まる
		infoqueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		D3D12_MESSAGE_ID denyids[] =
		{
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		//抑制するレベル
		D3D12_MESSAGE_SEVERITY secerities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyids);
		filter.DenyList.pIDList = denyids;
		filter.DenyList.NumSeverities = _countof(secerities);
		filter.DenyList.pSeverityList = secerities;
		//指定したメッセージの表示を抑制する
		infoqueue->PushStorageFilter(&filter);

		//解放
		infoqueue->Release();
	}


#endif // !_DEBUG
	//コマンドキューを生成する
	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc,
		IID_PPV_ARGS(&commandQueue));

	assert(SUCCEEDED(hr));

	//コマンドアロケータを生成する
	ID3D12CommandAllocator* commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(hr));

	//コマンドリストを作成する
	ID3D12GraphicsCommandList* commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr,
		IID_PPV_ARGS(&commandList));
	assert(SUCCEEDED(hr));

	//スラップチェインを作成する

	IDXGISwapChain4* swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth;//画面幅
	swapChainDesc.Height = kClientHeight;//画面高さ
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//色の形式
	swapChainDesc.SampleDesc.Count = 1;//マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain));
	assert(SUCCEEDED(hr));


	//DescriptorHeap

	//rtv用ののヒーブでくりぷの数は２RTV はShader内で触るものではないのでShaderVisbleはfalse
	ID3D12DescriptorHeap* rtvDescrriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	//SRV用の
	ID3D12DescriptorHeap* srvDescrriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	//DSV用のHeap
	ID3D12DescriptorHeap* dsvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);


	//swapChainからREsoucesを引っ張ってくる
	ID3D12Resource* swapChainResouces[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResouces[0]));
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResouces[1]));
	assert(SUCCEEDED(hr));

	//rtvの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//出力結果をSRGBに変換して書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;//2dテクスチャとして書き込み

	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescrriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

	//まず一つ目を作る
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResouces[0], &rtvDesc, rtvHandles[0]);
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//二つ目を作る
	device->CreateRenderTargetView(swapChainResouces[1], &rtvDesc, rtvHandles[1]);

	//初期値で0Fenceを作る

	ID3D12Fence* fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));


	//fenceのsignalを持つためのイベントを作成する

	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);

	MSG msg{};

	//dxcompilerを初期化

	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	//現地点のincに対応するための設定が行っておく
	IDxcIncludeHandler* includeHander = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHander);
	assert(SUCCEEDED(hr));

	//rootsignantrue作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignatrue{};

	descriptionRootSignatrue.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


	//descriptorRangeによる一括設定

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;//0から始まる
	descriptorRange[0].NumDescriptors = 1;//数は一つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//srvを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;//自動計算

	//Rootparameter作成　複数設定できるので配列 今回は結果が一つだけなので長さが1の配列

	D3D12_ROOT_PARAMETER rootParmeters[4] = {};
	rootParmeters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//CBVを使う
	rootParmeters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//pixeShaderで使う
	rootParmeters[0].Descriptor.ShaderRegister = 0;//レジスタ番号と0バインド

	rootParmeters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//CVBを使う
	rootParmeters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;//vertexShaerで使う
	rootParmeters[1].Descriptor.ShaderRegister = 0;//レジスタ番号0を使う

	rootParmeters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//descriptorTableを使う
	rootParmeters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParmeters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParmeters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);//tableで利用する数

	//05_00で追加

	rootParmeters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//CBVを使う
	rootParmeters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderを使う
	rootParmeters[3].Descriptor.ShaderRegister = 1;//レジスタ番号1を使う


	descriptionRootSignatrue.pParameters = rootParmeters;//ルートパラメーターへのポインタ
	descriptionRootSignatrue.NumParameters = _countof(rootParmeters);//配列の長さ

	//samplerの設定お行う
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;//バイリニアフィルター
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//0 ~1の範囲外をリピート 
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//比較
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignatrue.pStaticSamplers = staticSamplers;
	descriptionRootSignatrue.NumStaticSamplers = _countof(staticSamplers);





	//マテリアル用のリソースを作る　今回はcolor一つ分のサイズを用意する
	ID3D12Resource* materialResouces = createBufferResouces(device, sizeof(Vector4));

	//マテリアル用のデータを書き込む
	Vector4* materialData = nullptr;

	//書き込むためのアドレスを取得
	materialResouces->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

	//今回は赤を書き込んでみる
	*materialData = Vector4(1.0f, 0.0f, 0.0f, 1.0f);




	//シアライズしてばいなりにする
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignatrue, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr))
	{
		Log(logStrem, reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	ID3D12RootSignature* rootsignatrue = nullptr;
	hr = device->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootsignatrue));
	assert(SUCCEEDED(hr));

	//inputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;


	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	//BlendStateの設定
	D3D12_BLEND_DESC blendDescs{};
	blendDescs.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	//rasiterzerstateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	//裏面時計回りに表示しない

	//Noneにすると両面描画
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

	//三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//shaderをコンパイルする
	IDxcBlob* vertexShaderBlob = CompileShander(L"Object3d.VS.hlsl",
		L"vs_6_0", dxcUtils, dxcCompiler, includeHander, logStrem);
	assert(vertexShaderBlob != nullptr);
	IDxcBlob* pixelShaderBlob = CompileShander(L"Object3d.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHander, logStrem);
	assert(pixelShaderBlob != nullptr);

	//psoを作成する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootsignatrue;
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
	vertexShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
	pixelShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.BlendState = blendDescs;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//Depthの機能を無効化する
	depthStencilDesc.DepthEnable = true;
	//書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//比較関係はLessEqual　つまり透ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	//DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//巻き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//利用するトロポジの情報のタイプ
	graphicsPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//どのように画面に色を打ち込むのか設定
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//実際に生成
	ID3D12PipelineState* graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	//頂点とリソース用のヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;


	//04-00で新しくつくる

	//Sprite用の頂点リソースを作る

	ID3D12Resource* vertexResourceSprite = createBufferResouces(device, sizeof(VertexData) * 6);

	//頂点バッファビューを作成する

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};

	//リソースの先端アドレスから使う
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();

	//使用するリソースのサイズは頂点3つ分サイズ
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;

	//1頂点当たりのサイズ
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	//頂点リソースサイズデータに書き込む
	VertexData* vertexDataSprite = nullptr;

	//書き込むためのアドレスの取得
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<VOID**>(&vertexDataSprite));

	//1枚目の三角形

	//左
	vertexDataSprite[0].position = { 0.0f, 360.0f,0.0f,1.0f };
	vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
	//上
	vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	vertexDataSprite[1].texcoord = { 0.0f, 0.0f };

	//右
	vertexDataSprite[2].position = { 640.0f,360.0f, 0.0f, 1.0f };
	vertexDataSprite[2].texcoord = { 1.0f, 1.0f };

	//2枚目の三角形

	vertexDataSprite[3].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[3].texcoord = { 0.0f,0.0f };

	vertexDataSprite[4].position = { 640.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[4].texcoord = { 1.0f,0.0f };

	vertexDataSprite[5].position = { 640.0f,360.0f,0.0f,1.0f };
	vertexDataSprite[5].texcoord = { 1.0f,1.0f };


	//*Index用//

	//リソース作成
	ID3D12Resource* indexResourceSprite = createBufferResouces(device, sizeof(uint32_t) * 6);
	//BufferView
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	//リソースをアドレスから使う
	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	//リソースのサイズ
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	//インデックスはuint32_tとする
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	//データ書き込み
	uint32_t* indexDataSprite = nullptr;
	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));

	//インデックス初期化
	indexDataSprite[0] = 0;
	indexDataSprite[1] = 1;
	indexDataSprite[2] = 2;
	indexDataSprite[3] = 1;
	indexDataSprite[4] = 3;
	indexDataSprite[5] = 2;

	//*index終了//





	//球体

	//Spehere用の頂点情報
	const uint32_t kSubdivision = 16;
	const uint32_t sphereVertexNum = kSubdivision * kSubdivision * 6;



	//Sphere用の頂点リソースを作る
	ID3D12Resource* vertexResourceSphere = createBufferResouces(device, sizeof(VertexData) * sphereVertexNum);

	//Sphereバッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere{};

	// リソースの先端アドレスから使う
	vertexBufferViewSphere.BufferLocation = vertexResourceSphere->GetGPUVirtualAddress();

	//使用するリソースのサイズは頂点3つ分サイズ
	vertexBufferViewSphere.SizeInBytes = sizeof(VertexData) * sphereVertexNum;

	//1頂点当たりのサイズ
	vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);

	//球体リソースサイズデータに書き込む
	VertexData* vertexDataSphere = nullptr;

	//書き込むためのアドレスの取得
	vertexResourceSphere->Map(0, nullptr, reinterpret_cast<VOID**>(&vertexDataSphere));

	//Sprite用のTransformationMatrix用のリソースを作る Matrix4x4
	ID3D12Resource* transformationMatrixResourceSphere = createBufferResouces(device, sizeof(Matrix4x4));

	//	データを書き込む
	Matrix4x4* transformationMatrixDataSphere = nullptr;

	//書き込むアドレスを取得
	transformationMatrixResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSphere));

	//単位行列を書き込んでおく
	*transformationMatrixDataSphere = makeIdentity4x4();



	//緯度分割1つ分の角度
	const float kLonEvery = std::numbers::pi_v<float>*2.0f / float(kSubdivision);
	//緯度分割1つ分の角度θ
	const float kLatEvery = std::numbers::pi_v<float> / float(kSubdivision);



	//緯度の方向に分割
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex;//0
		//緯度の方向に分割しながら線を引く
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;
			float lon = lonIndex * kLonEvery;

			VertexData VertA = {
		{
		std::cosf(lat) * std::cosf(lon),
		std::sinf(lat),
		std::cosf(lat) * std::sinf(lon),
		1.0f
		},
	{
		float(lonIndex) / float(kSubdivision),
		1.0f - float(latIndex) / float(kSubdivision)
	},
		{
			//normal
		std::cosf(lat) * std::cosf(lon),
		std::sinf(lat),
		std::cosf(lat) * std::sinf(lon),
		}

			};
			VertexData VertB = {
				{
				std::cosf(lat + kLatEvery) * std::cosf(lon),
				std::sinf(lat + kLatEvery),
				std::cosf(lat + kLatEvery) * std::sinf(lon),
				1.0f
				},
				{
					float(lonIndex) / float(kSubdivision),
					1.0f - float(latIndex + 1.0f) / float(kSubdivision)
				},
				{
					//normal
				std::cosf(lat + kLatEvery) * std::cosf(lon),
				std::sinf(lat + kLatEvery),
				std::cosf(lat + kLatEvery) * std::sinf(lon),
				}
			};
			VertexData VertC = {
				{
				std::cosf(lat) * std::cosf(lon + kLonEvery),
				std::sinf(lat),
				std::cosf(lat) * std::sinf(lon + kLonEvery),
				1.0f
				},
				{
					float(lonIndex + 1.0f) / float(kSubdivision),
					1.0f - float(latIndex) / float(kSubdivision)
				},
				//normal
				{
				std::cosf(lat) * std::cosf(lon + kLonEvery),
				std::sinf(lat),
				std::cosf(lat) * std::sinf(lon + kLonEvery),
				}
			};
			VertexData VertD = {
				{
				std::cosf(lat + kLatEvery) * std::cosf(lon + kLonEvery),
				std::sinf(lat + kLatEvery),
				std::cosf(lat + kLatEvery) * std::sinf(lon + kLonEvery),
				1.0f
				},
				{
					float(lonIndex + 1.0f) / float(kSubdivision),
					1.0f - float(latIndex + 1.0f) / float(kSubdivision)
				},
				//normal
				{std::cosf(lat + kLatEvery) * std::cosf(lon + kLonEvery),
				std::sinf(lat + kLatEvery),
				std::cosf(lat + kLatEvery) * std::sinf(lon + kLonEvery),
				}
			};
			vertexDataSphere[start + 0] = VertA;
			vertexDataSphere[start + 1] = VertB;
			vertexDataSphere[start + 2] = VertC;
			vertexDataSphere[start + 3] = VertC;
			vertexDataSphere[start + 4] = VertB;
			vertexDataSphere[start + 5] = VertD;
		}
	}

	const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);



	//05_03追加


	ID3D12Resource* vertexResouces = createBufferResouces(device, sizeof(VertexData) * 6);

	//頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	//リソースの先端アドレスから使う
	vertexBufferView.BufferLocation = vertexResouces->GetGPUVirtualAddress();

	//使用するリソースのサイズは頂点3つ分サイズ
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;

	//1頂点当たりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);


	//WVPのリソースを作る matrix4*4 一つ分のサイズを用意する
	ID3D12Resource* wvpResouces = createBufferResouces(device, sizeof(TransformationMatrix));

	//データ書き込む
	TransformationMatrix* wvpData = nullptr;

	Transform transform = { {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };

	//書き込むためのアドレス
	wvpResouces->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	wvpData->WVP = makeIdentity4x4();
	wvpData->World = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

	//頂点リソースサイズデータに書き込む
	VertexData* vertexData = nullptr;

	//書き込むためのアドレスの取得
	vertexResouces->Map(0, nullptr, reinterpret_cast<VOID**>(&vertexData));






	//光源用のリソース生成開始

	//Sprite用のマテリアル用リソースを作る　
	ID3D12Resource* materialResourcesSprite = createBufferResouces(device, sizeof(Material));

	//マテリアル用のデータを書き込む
	Material* materialDataSprite = nullptr;

	//書き込むためのアドレスを取得
	materialResourcesSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));

	//生成終了

	//球体フラグ生成

	//今回は白を書き込んでみる
	materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f);

	materialDataSprite->enableLighting = true;

	//フラグ生成処理終了




	//Sprite用のTransformationMatrix用のリソースを作る Matrix4x4

	ID3D12Resource* transformationMatrixResourceSprite = createBufferResouces(device, sizeof(Matrix4x4));

	//	データを書き込む

	Matrix4x4* transformationMatrixDataSprite = nullptr;

	//書き込むアドレスを取得

	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));

	//単位行列を書き込んでおく

	*transformationMatrixDataSprite = makeIdentity4x4();

	//ビューボート
	D3D12_VIEWPORT viewport{};

	//クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = kClientWidth;
	viewport.Height = kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//シザー矩形
	D3D12_RECT scissorRect{};
	scissorRect.left = 0;
	scissorRect.right = kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;

	//imguiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(device,
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescrriptorHeap,
		srvDescrriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescrriptorHeap->GetGPUDescriptorHandleForHeapStart());


	//2枚目のTextureを読んで転送する
	DirectX::ScratchImage mipImages2 = LoadTexture("resources/monsterBall.png");
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
	ID3D12Resource* textureResource2 = CreateTextureResource(device, metadata2);
	ID3D12Resource* intermediateResource2 = UploadTextureData(textureResource2, mipImages2, device, commandList);

	//mataDataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = metadata2.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2dテクスチャ
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescrriptorHeap, descriptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescrriptorHeap, descriptorSizeSRV, 2);

	//srvを作成する
	device->CreateShaderResourceView(textureResource2, &srvDesc2, textureSrvHandleCPU2);





	//textureを読んで転送する
	DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	ID3D12Resource* textureResource = CreateTextureResource(device, metadata);
	ID3D12Resource* intermediateResource = UploadTextureData(textureResource, mipImages, device, commandList);
	//DepthStencilTextureをウィンドウのサイズで作成
	ID3D12Resource* depthStencilResource = CreateDepthStencilTextureResource(device, kClientWidth, kClientHeight);

	//mataDataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata2.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2dテクスチャ
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	//SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = GetCPUDescriptorHandle(srvDescrriptorHeap, descriptorSizeSRV, 1);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = GetGPUDescriptorHandle(srvDescrriptorHeap, descriptorSizeSRV, 1);

	//srvを作成する
	device->CreateShaderResourceView(textureResource, &srvDesc, textureSrvHandleCPU);



	//DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//Format 基本的にResourceに合わせる
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2dTexture

	//DSVHeapの先頭に作る
	device->CreateDepthStencilView(depthStencilResource, &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


	////SRVを作成するDescriptorHeapの場所を求める
	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = srvDescrriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = srvDescrriptorHeap->GetGPUDescriptorHandleForHeapStart();



	////先端はimguiを作成するDescriptorの場所を決める
	//textureSrvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//textureSrvHandleGPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//


	//初期化

	Transform cameraTransform = { {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, -10.0f} };
	//Sprite用のを作成
	Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };



	//光の向き
	ID3D12Resource* materialResourceDirection = createBufferResouces(device, sizeof(DirectionalLight));






	DirectionalLight* directionalLightData = nullptr;
	materialResourceDirection->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = { 0.0f,-1.0f,0.0f };
	directionalLightData->intensity = 1.0f;


	//メインループ

	//windowの×ボタンが押されるまでループ
	while (msg.message != WM_QUIT)
	{





		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else
		{

			bool temp_enableLighting = (materialDataSprite->enableLighting != 0);

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			ImGui::ShowDemoWindow();


			//ゲームの処理
			ImGui::Begin("MaterialColor");

			ImGui::ColorEdit4("color", &(materialData)->x);
			ImGui::ColorEdit3("LightColor", &directionalLightData->color.x);

			//切り替え用のImGuiを追加
			ImGui::Checkbox("useMonsterBall", &useMonsterBall);

			if (ImGui::Checkbox("enableLighting", &temp_enableLighting)) {
				materialDataSprite->enableLighting = temp_enableLighting ? 1 : 0;
			}

			ImGui::SliderFloat3("LightDirection", &directionalLightData->direction.x, -1.0f, 1.0f);
			ImGui::DragFloat("Intensity", &directionalLightData->intensity);

			ShowSRTWindow(transform);

			ImGui::End();




			transform.rotate.y += 0.03f;
			Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

			//カメラの処理
			Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
			wvpData->WVP = worldViewProjectionMatrix;
			wvpData->World = worldMatrix;

			//04-00用にWorldViewProjectionMatrixを作る

			Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate,
				transformSprite.translate);
			Matrix4x4 viewMatrixSprite = makeIdentity4x4();
			Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(kClientWidth), float(kClientHeight), 0.0f, 100.0f);
			Matrix4x4 worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
			*transformationMatrixDataSprite = worldViewProjectionMatrixSprite;

			//画面のクリア処理

			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

			//TransitonBarrierの設定
			D3D12_RESOURCE_BARRIER barrier{};

			//今回のバリアはtransition
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			//Noneにしておく
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

			//バリアを張る対象のリソース現在のバックバッファに対して行う
			barrier.Transition.pResource = swapChainResouces[backBufferIndex];

			//転移前の(現在)のResoucesStare
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;

			//転移後のResourceState
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

			//transSitiionBarrierを張る
			commandList->ResourceBarrier(1, &barrier);

			//描画先のRTVとDSVを設定する
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();


			//画面先のrtvを設定する
			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);

			float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };

			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
			//描画用のDscriptorHeapの設定

			commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescrriptorHeap };
			commandList->SetDescriptorHeaps(1, descriptorHeaps);

			//コマンドを積む
			commandList->RSSetViewports(1, &viewport);
			commandList->RSSetScissorRects(1, &scissorRect);

			//球体のIASet
			//rootsignaltrueを設定　psoに設定しているけど別途設定が必要
			commandList->SetGraphicsRootSignature(rootsignatrue);
			commandList->SetPipelineState(graphicsPipelineState);
			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSphere);//VBVを設定する


			//形状を設定psoに設定しているものとはまた別　同じものを設定するトロ考えておけば良い
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			//マテリアルcBufferの場所設定
			commandList->SetGraphicsRootConstantBufferView(0, materialResourcesSprite->GetGPUVirtualAddress());

			commandList->SetGraphicsRootConstantBufferView(3, materialResourceDirection->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(1, wvpResouces->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);

			//作画
			commandList->DrawInstanced(sphereVertexNum, 1, 0, 0);

			commandList->IASetIndexBuffer(&indexBufferViewSprite);

			commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

			//commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

			//

			////Spriteの描画
			//commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);

			////TransformationMatrixCBufferの場所を設定
			//commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
			// 
			////作画
			//commandList->DrawInstanced(6, 1, 0, 0);

			ImGui::Render();



			//実際のcommmandList残り時間imguiの描画コマンドを積む
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

			//renderTarGetからPresentにする
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

			//transSitiionBarrierを張る
			commandList->ResourceBarrier(1, &barrier);

			hr = commandList->Close();
			assert(SUCCEEDED(hr));

			ID3D12CommandList* commandLists[] = { commandList };
			commandQueue->ExecuteCommandLists(1, commandLists);
			swapChain->Present(1, 0);


			//Fenceの更新
			fenceValue++;

			//GPUがそこまでたどり着いた時に Fenceの値を指定した値に代入するようにsignalを送る
			commandQueue->Signal(fence, fenceValue);

			if (fence->GetCompletedValue() < fenceValue)
			{
				//指定したsignal似たとりついていないのでたどり着くまでに待つようにイベントを指定する
				fence->SetEventOnCompletion(fenceValue, fenceEvent);
				//イベントを待つ
				WaitForSingleObject(fenceEvent, INFINITE);
			}


			hr = commandAllocator->Reset();
			assert(SUCCEEDED(hr));
			hr = commandList->Reset(commandAllocator, nullptr);
			assert(SUCCEEDED(hr));



		}


	}


	Log(logStrem, "Hello,DirectX!\n");
	Log(logStrem,
		ConvertString(
			std::format(
				L"clientSize:{} {}\n",
				kClientWidth,
				kClientHeight)));

	CloseWindow(hwnd);

	//解放処理
	CloseHandle(fenceEvent);
	fence->Release();
	rtvDescrriptorHeap->Release();
	srvDescrriptorHeap->Release();
	swapChainResouces[0]->Release();
	swapChainResouces[1]->Release();
	swapChain->Release();
	commandList->Release();
	commandAllocator->Release();
	commandQueue->Release();
	device->Release();
	useAsapter->Release();
	dxgiFactory->Release();
	//使っていないのでコメントアウト
	vertexResouces->Release();
	graphicsPipelineState->Release();
	signatureBlob->Release();
	if (errorBlob)
	{
		errorBlob->Release();
	}
	rootsignatrue->Release();
	pixelShaderBlob->Release();
	vertexShaderBlob->Release();
	materialResouces->Release();
	//使っていないのでコメントアウト
	wvpResouces->Release();
	textureResource->Release();
	textureResource2->Release();
	intermediateResource->Release();
	intermediateResource2->Release();
	depthStencilResource->Release();
	dsvDescriptorHeap->Release();
	vertexResourceSprite->Release();
	transformationMatrixResourceSprite->Release();
	vertexResourceSphere->Release();
	transformationMatrixResourceSphere->Release();
	materialResourcesSprite->Release();
	materialResourceDirection->Release();
	wvpResouces->Release();
	vertexResouces->Release();
	indexResourceSprite->Release();
#ifdef _DEBUG

	debugController->Release();
#endif // _DEBUG

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	//comの終了時
	CoUninitialize();

	//リソースチェック
	IDXGIDebug* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
	{
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}

	return 0;
}