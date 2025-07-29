#include <windows.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <chrono>
#include <format>
#include <fstream>
#include <sstream>
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
#include<wrl.h>
#include <xaudio2.h>
#define DIRECTINPUT_VERSION 0x0800//DirectInputのバージョン指定
#include <dinput.h>


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib,"xaudio2.lib")
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

using Microsoft::WRL::ComPtr;

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
	float padding[3];
	Matrix4x4 uvTransform;
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


//mtl読み込み用
struct MaterialData
{
	std::string textureFilePath;
};

//Obj読み込み用
struct ModelData
{
	std::vector<VertexData>vertices;
	MaterialData material;
};

//07-00で追加

//チャンクヘッダ
struct ChunkHeader
{
	char id[4];//チャンクごとのID
	int32_t size;//チャンクサイズ
};

//RIFFヘッダチャンク
struct RiffHeader
{
	ChunkHeader chunk;//"Riff"
	char type[4];//"Wave"
};

struct FormatChunk
{
	ChunkHeader chunk;
	WAVEFORMATEX fmt;

};

//音声データ
struct  SoundData
{
	//波形フォーマット
	WAVEFORMATEX wfex;
	//バッファの先頭アドレス
	BYTE* pBuffer;
	//バッファのサイズ
	unsigned int bufferSize;

};

struct D3DResourceLeakChecker {
	~D3DResourceLeakChecker()
	{
		//リソースチェック
		ComPtr<IDXGIDebug> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
		{
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);

		}
	}
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

Vector3 Normalize(const Vector3& v) {
	float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (length == 0.0f)
		return { 0.0f, 0.0f, 0.0f };
	return { v.x / length, v.y / length, v.z / length };
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
ComPtr<ID3D12Resource> createBufferResouces(ComPtr<ID3D12Device> device, size_t sizeInBytes)
{
	//頂点とリソース用のヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; //3_0Exでの変更点

	//頂点リソースの設定
	D3D12_RESOURCE_DESC vertResoucesDesc{};

	//バッファーリソーステクスチャの場合は別の指定をする
	vertResoucesDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertResoucesDesc.Width = sizeInBytes;

	//バッファの場合は1にする
	vertResoucesDesc.Height = 1;
	vertResoucesDesc.DepthOrArraySize = 1;
	vertResoucesDesc.MipLevels = 1;
	vertResoucesDesc.SampleDesc.Count = 1;

	//バッファの場合はこれをする決まり
	vertResoucesDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//実際に頂点リソースを作る
	ComPtr<ID3D12Resource> vertexResouces = nullptr;
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

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
	ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible
)
{
	ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
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

ComPtr<IDxcBlob> CompileShander(
	const std::wstring& filePath,
	const wchar_t* profile,
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxCompiler,
	IDxcIncludeHandler* includeHandler,
	std::ostream& os)
{
	//hlslファイルを読み込む
	Log(os, ConvertString(std::format(L"Begin CompileShader,path:{}, profile:{}\n", filePath, profile)));
	ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
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

	ComPtr<IDxcResult> shaderResult = nullptr;
	hr = dxCompiler->Compile(
		&shaderSoucesBuffer,
		argument,
		_countof(argument),
		includeHandler,
		IID_PPV_ARGS(&shaderResult));

	assert(SUCCEEDED(hr));
	//警告エラーが出てないか確認する
	ComPtr<IDxcBlobUtf8> shanderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shanderError), nullptr);
	if (shanderError != nullptr && shanderError->GetStringLength() != 0)
	{
		Log(os, shanderError->GetStringPointer());
		assert(false);

	}
	//Compile結果を受けて返す
	ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	Log(os, ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));

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

ComPtr<ID3D12Resource> CreateTextureResource(ComPtr<ID3D12Device> device, const DirectX::TexMetadata& metadata)
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
	ComPtr<ID3D12Resource> resouce = nullptr;
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

ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ComPtr<ID3D12Device> device, int32_t width, int32_t height)
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
	ComPtr<ID3D12Resource> resource = nullptr;
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
ComPtr<ID3D12Resource> UploadTextureData(ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages, ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList)
{
	std::vector<D3D12_SUBRESOURCE_DATA>subresources;
	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
	ComPtr<ID3D12Resource> intermediateResource = createBufferResouces(device.Get(), intermediateSize);
	UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());
	//Textureへの転送後は利用できるようにD3D12_RESOURCE_STATE_COPY_DEST_RESOURCE_STATE_GENERIC_READへResourceStateを変更する

	//新しく作ったBarrier
	D3D12_RESOURCE_BARRIER barrier{};

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;

	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;

}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index)
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

#pragma region MaterialData

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename)
{
	MaterialData materialData;//構築するMaterialData
	std::string line;//ファイルから読んだ1行を格納するもの
	std::ifstream file(directoryPath + "/" + filename);//ファイルを開く
	assert(file.is_open());//とりあえず開けなかったら止める

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		//identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			//連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}

	}

	return materialData;
}

#pragma endregion



#pragma region ModelData obj

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename)
{
	//1::変数宣言
	ModelData modelData;//構築するモデルデータ
	std::vector<Vector4> positions;//位置
	std::vector<Vector3> normals;//法線
	std::vector<Vector2> texcoords;//テクスチャ座標
	std::string line;//ファイルから読んだ1行を格納するもの
	VertexData triangle[3];

	//2::ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);//ファイルを開く
	assert(file.is_open());

	//3ファイルを読み込む
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;//先頭の識別子を読む

		//identifierに応じた処理
		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			/*position.x *= -1.0f;*/
			/*position.y *= -1.0f;*/
			/*position.z *= -1.0f;*/
			position.w = 1.0f;
			positions.push_back(position);
		} else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
		} else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			/*normal.x *= -1.0f;*/
			/*normal.y *= -1.0f;*/
			normals.push_back(normal);
		} else if (identifier == "f") {
			//面は三角形限定　その他は未対応
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				//頂点の要素へのIndexは「位置/UV/法線」で格納されているので分離してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');//区切りでindexを読んでいく
					elementIndices[element] = std::stoi(index);
				}
				//要素へのIndexから、実際の要素の値を取得して、頂点を構築する

				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				VertexData vertex = { position,texcoord,normal };
				modelData.vertices.push_back(vertex);
				triangle[faceVertex] = { position,texcoord,normal };

			}
			//頂点を逆順で登録することで、回り順を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		} else if (identifier == "mtllib") {
			//materialTemplateLibraryファイルの名前を格納する
			std::string materialFilename;
			s >> materialFilename;
			//基本的にobjファイルと同一階層にmtlは存在させるのでディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);

		}
	}
	return modelData;
}

#pragma endregion


SoundData SoundLoadWave(const char* filename)
{

	//①
	//ファイル入力ストリームのインスタンス
	std::ifstream file;
	//wavファイルをバイナリモードで開く
	file.open(filename, std::ios_base::binary);
	//ファイルオープン失敗を検出する
	assert(file.is_open());

	//②
	//Riffヘッダーの読み込み
	RiffHeader riff;
	file.read((char*)&riff, sizeof(riff));
	//ファイルがRiffかチェック
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		assert(0);
	}

	//タイプがWaveかチェック
	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}

	//フォーマットチャンクの読み込み
	FormatChunk format = {};
	ChunkHeader chunk = {};

	while (true) {
		file.read((char*)&chunk, sizeof(chunk));
		if (file.eof()) {
			assert(0 && "fmt チャンクが見つかりませんでした");
		}

		if (strncmp(chunk.id, "fmt ", 4) == 0) {
			assert(chunk.size <= sizeof(format.fmt));
			format.chunk = chunk;
			file.read((char*)&format.fmt, chunk.size);
			break;
		} else {
			// 必要ないチャンクはスキップ
			file.seekg(chunk.size, std::ios_base::cur);
		}
	}
	//Dataチャンクの読み込み
	ChunkHeader data;
	file.read((char*)&data, sizeof(data));
	//JUNKチャンクを検出した場合
	if (strncmp(data.id, "JUNK", 4) == 0) {
		//読み取り位置をJUNKチャンクの終わりまで進める
		file.seekg(data.size, std::ios_base::cur);
		//再読み込み
		file.read((char*)&data, sizeof(data));
	}

	//③
	if (strncmp(data.id, "data", 4) != 0) {
		assert(0);
	}

	//Dataチャンクのデータ部(波形データ)の読み込み
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);

	//Waveファイルを閉じる
	file.close();

	//④
	//returnするための音声データ
	SoundData soundData = {};

	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;

	return soundData;
}

void SoundUnLoad(SoundData* soundData)
{
	//バッファのメモリ解放
	delete[]soundData->pBuffer;

	soundData->pBuffer = 0;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData)
{
	HRESULT result;

	//波形フォーマットを元にSourceVoiceの生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	//再生する波形データの生成
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	//波形データの再生
	result = pSourceVoice->SubmitSourceBuffer(&buf);
	result = pSourceVoice->Start();

}


bool useMonsterBall = true;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

#pragma region DirectX初期化処理

	SetUnhandledExceptionFilter(ExportDump);

	//comの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	D3DResourceLeakChecker leakCheck;
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	ComPtr<IXAudio2>xAudio2;
	IXAudio2MasteringVoice* masterVoice;




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

	//xAudio2インスタンス生成
	HRESULT result;
	result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	result = xAudio2->CreateMasteringVoice(&masterVoice);


	////前フレームのキー入力
	//BYTE prevKey[256] = {};

#ifdef _DEBUG
	ComPtr<ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(true);
	}




#endif // !_DEBUG






	//関数が成功したかどうかSUCCEEDマクロで判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(hr));

	//仕様するアダプタ用生成の変数。最初にnullptrを入れておく
	ComPtr<IDXGIAdapter4> useAsapter = nullptr;

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


	//機能レベルとログの出力
	D3D_FEATURE_LEVEL featrueLevels[] =
	{ D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0 };
	const char* featrueLevelStrings[] = { "12.1", "12.1", "12.0" };
	for (size_t i = 0; i < _countof(featrueLevels); ++i)
	{
		//採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAsapter.Get(), featrueLevels[i], IID_PPV_ARGS(&device));
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

	ComPtr<ID3D12InfoQueue> infoqueue = nullptr;

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

	}


#endif // !_DEBUG
	//コマンドキューを生成する
	ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc,
		IID_PPV_ARGS(&commandQueue));

	assert(SUCCEEDED(hr));

	//コマンドアロケータを生成する
	ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(hr));

	//コマンドリストを作成する
	ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr,
		IID_PPV_ARGS(&commandList));
	assert(SUCCEEDED(hr));

	//スラップチェインを作成する

	ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth;//画面幅
	swapChainDesc.Height = kClientHeight;//画面高さ
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//色の形式
	swapChainDesc.SampleDesc.Count = 1;//マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
	assert(SUCCEEDED(hr));


	//DescriptorHeap

	//rtv用ののヒーブでくりぷの数は２RTV はShader内で触るものではないのでShaderVisbleはfalse
	ComPtr<ID3D12DescriptorHeap> rtvDescrriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	//SRV用の
	ComPtr<ID3D12DescriptorHeap> srvDescrriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	//DSV用のHeap
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);


	//swapChainからREsoucesを引っ張ってくる
	ComPtr<ID3D12Resource> swapChainResouces[2] = { nullptr };
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
	device->CreateRenderTargetView(swapChainResouces[0].Get(), &rtvDesc, rtvHandles[0]);
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//二つ目を作る
	device->CreateRenderTargetView(swapChainResouces[1].Get(), &rtvDesc, rtvHandles[1]);

	//初期値で0Fenceを作る

	ComPtr<ID3D12Fence> fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));


	//fenceのsignalを持つためのイベントを作成する

	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);

	MSG msg{};

	//dxcompilerを初期化

	ComPtr<IDxcUtils> dxcUtils = nullptr;
	ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	//現地点のincに対応するための設定が行っておく
	ComPtr<IDxcIncludeHandler> includeHander = nullptr;
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


	//シアライズしてばいなりにする
	ComPtr<ID3DBlob> signatureBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignatrue, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr))
	{
		Log(logStrem, reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	ComPtr<ID3D12RootSignature> rootsignatrue = nullptr;
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
	ComPtr<IDxcBlob> vertexShaderBlob = CompileShander(L"Object3d.VS.hlsl",
		L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHander.Get(), logStrem);
	assert(vertexShaderBlob != nullptr);
	ComPtr<IDxcBlob> pixelShaderBlob = CompileShander(L"Object3d.PS.hlsl", L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHander.Get(), logStrem);
	assert(pixelShaderBlob != nullptr);

	//psoを作成する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootsignatrue.Get();
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
	ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	//頂点とリソース用のヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	//DirectInputの初期化
	IDirectInput8* directInput = nullptr;
	result = DirectInput8Create(wc.hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

	IDirectInputDevice8* keyboard = nullptr;
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	//入力データ形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	//排他制御レベルのセット
	result = keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));

	keyboard->Acquire();


#pragma endregion

#pragma region Sprite

	//Spriteの処理

		//04-00で新しくつくる

		//Sprite用の頂点リソースを作る

	ComPtr<ID3D12Resource> vertexResourceSprite = createBufferResouces(device.Get(), sizeof(VertexData) * 6);

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

	//左上
	vertexDataSprite[0].position = { 0.0f, 360.0f,0.0f,1.0f };
	vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
	//左下
	vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	vertexDataSprite[1].texcoord = { 0.0f, 0.0f };

	//右下
	vertexDataSprite[2].position = { 640.0f,360.0f, 0.0f, 1.0f };
	vertexDataSprite[2].texcoord = { 1.0f, 1.0f };

	//右上
	vertexDataSprite[3].position = { 640.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[3].texcoord = { 1.0f,0.0f };


#pragma endregion 

#pragma region Triangle

	//ID3D12Resource* triangleVertexResouces = createBufferResouces(device, sizeof(VertexData) * 6);

	////頂点バッファビューを作成する
	//D3D12_VERTEX_BUFFER_VIEW triangleVertexBufferView{};

	////リソースの先端アドレスから使う
	//triangleVertexBufferView.BufferLocation = triangleVertexResouces->GetGPUVirtualAddress();

	////使用するリソースのサイズは頂点3つ分サイズ
	//triangleVertexBufferView.SizeInBytes = sizeof(VertexData) * 6;

	////1頂点当たりのサイズ
	//triangleVertexBufferView.StrideInBytes = sizeof(VertexData);

	////頂点リソースサイズデータに書き込む
	//VertexData* vertexData = nullptr;

	////書き込むためのアドレスの取得
	//vertexResouces->Map(0, nullptr, reinterpret_cast<VOID**>(&vertexData));

#pragma endregion 

#pragma region IndexSprite
	//*Index用//

//リソース作成
	ComPtr<ID3D12Resource> indexResourceSprite = createBufferResouces(device.Get(), sizeof(uint32_t) * 6);
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

#pragma endregion 

#pragma region BeforeSphere


	////Spehere用の頂点情報
	//const uint32_t kSubdivision = 16;
	//const uint32_t sphereVertexNum = kSubdivision * kSubdivision * 6;

	////Sphere用の頂点リソースを作る
	//ID3D12Resource* vertexResourceSphere = createBufferResouces(device, sizeof(VertexData) * sphereVertexNum);

	////Sphereバッファビューを作成する
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere{};

	//// リソースの先端アドレスから使う
	//vertexBufferViewSphere.BufferLocation = vertexResourceSphere->GetGPUVirtualAddress();

	////使用するリソースのサイズは頂点3つ分サイズ
	//vertexBufferViewSphere.SizeInBytes = sizeof(VertexData) * sphereVertexNum;

	////1頂点当たりのサイズ
	//vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);

	////球体リソースサイズデータに書き込む
	//VertexData* vertexDataSphere = nullptr;

	////書き込むためのアドレスの取得
	//vertexResourceSphere->Map(0, nullptr, reinterpret_cast<VOID**>(&vertexDataSphere));

	////Sprite用のTransformationMatrix用のリソースを作る Matrix4x4
	//ID3D12Resource* transformationMatrixResourceSphere = createBufferResouces(device, sizeof(Matrix4x4));

	////	データを書き込む
	//Matrix4x4* transformationMatrixDataSphere = nullptr;

	////書き込むアドレスを取得
	//transformationMatrixResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSphere));

	////単位行列を書き込んでおく
	//*transformationMatrixDataSphere = makeIdentity4x4();

	////緯度分割1つ分の角度
	//const float kLonEvery = std::numbers::pi_v<float>*2.0f / float(kSubdivision);
	////緯度分割1つ分の角度θ
	//const float kLatEvery = std::numbers::pi_v<float> / float(kSubdivision);



	////緯度の方向に分割
	//for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
	//	float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex;//0
	//	//緯度の方向に分割しながら線を引く
	//	for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
	//		uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;
	//		float lon = lonIndex * kLonEvery;

	//		VertexData VertA = {
	//	{
	//	std::cosf(lat) * std::cosf(lon),
	//	std::sinf(lat),
	//	std::cosf(lat) * std::sinf(lon),
	//	1.0f
	//	},
	//{
	//	float(lonIndex) / float(kSubdivision),
	//	1.0f - float(latIndex) / float(kSubdivision)
	//},
	//	{
	//		//normal
	//	std::cosf(lat) * std::cosf(lon),
	//	std::sinf(lat),
	//	std::cosf(lat) * std::sinf(lon),
	//	}

	//		};
	//		VertexData VertB = {
	//			{
	//			std::cosf(lat + kLatEvery) * std::cosf(lon),
	//			std::sinf(lat + kLatEvery),
	//			std::cosf(lat + kLatEvery) * std::sinf(lon),
	//			1.0f
	//			},
	//			{
	//				float(lonIndex) / float(kSubdivision),
	//				1.0f - float(latIndex + 1.0f) / float(kSubdivision)
	//			},
	//			{
	//				//normal
	//			std::cosf(lat + kLatEvery) * std::cosf(lon),
	//			std::sinf(lat + kLatEvery),
	//			std::cosf(lat + kLatEvery) * std::sinf(lon),
	//			}
	//		};
	//		VertexData VertC = {
	//			{
	//			std::cosf(lat) * std::cosf(lon + kLonEvery),
	//			std::sinf(lat),
	//			std::cosf(lat) * std::sinf(lon + kLonEvery),
	//			1.0f
	//			},
	//			{
	//				float(lonIndex + 1.0f) / float(kSubdivision),
	//				1.0f - float(latIndex) / float(kSubdivision)
	//			},
	//			//normal
	//			{
	//			std::cosf(lat) * std::cosf(lon + kLonEvery),
	//			std::sinf(lat),
	//			std::cosf(lat) * std::sinf(lon + kLonEvery),
	//			}
	//		};
	//		VertexData VertD = {
	//			{
	//			std::cosf(lat + kLatEvery) * std::cosf(lon + kLonEvery),
	//			std::sinf(lat + kLatEvery),
	//			std::cosf(lat + kLatEvery) * std::sinf(lon + kLonEvery),
	//			1.0f
	//			},
	//			{
	//				float(lonIndex + 1.0f) / float(kSubdivision),
	//				1.0f - float(latIndex + 1.0f) / float(kSubdivision)
	//			},
	//			//normal
	//			{std::cosf(lat + kLatEvery) * std::cosf(lon + kLonEvery),
	//			std::sinf(lat + kLatEvery),
	//			std::cosf(lat + kLatEvery) * std::sinf(lon + kLonEvery),
	//			}
	//		};
	//		vertexDataSphere[start + 0] = VertA;
	//		vertexDataSphere[start + 1] = VertB;
	//		vertexDataSphere[start + 2] = VertC;
	//		vertexDataSphere[start + 3] = VertC;
	//		vertexDataSphere[start + 4] = VertB;
	//		vertexDataSphere[start + 5] = VertD;
	//	}
	//}



#pragma endregion 


#pragma region AfterSphere

	const uint32_t kSubdivision = 16;
	const uint32_t vertexCount = (kSubdivision + 1) * (kSubdivision + 1);
	const uint32_t indexCount = kSubdivision * kSubdivision * 6;



	// 頂点リソース作成
	ComPtr<ID3D12Resource> vertexResourceSphere = createBufferResouces(device.Get(), sizeof(VertexData) * vertexCount);

	// 頂点バッファビュー作成
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere{};
	vertexBufferViewSphere.BufferLocation = vertexResourceSphere->GetGPUVirtualAddress();
	vertexBufferViewSphere.SizeInBytes = sizeof(VertexData) * vertexCount;
	vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);

	// 頂点データ書き込み
	VertexData* vertexDataSphere = nullptr;
	vertexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSphere));

	const float kLonEvery = std::numbers::pi_v<float> *2.0f / float(kSubdivision);
	const float kLatEvery = std::numbers::pi_v<float> / float(kSubdivision);

	for (uint32_t latIndex = 0; latIndex <= kSubdivision; ++latIndex) {
		float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex;
		for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; ++lonIndex) {
			float lon = kLonEvery * lonIndex;

			uint32_t index = latIndex * (kSubdivision + 1) + lonIndex;

			vertexDataSphere[index].position = {
				std::cosf(lat) * std::cosf(lon),
				std::sinf(lat),
				std::cosf(lat) * std::sinf(lon),
				1.0f
			};
			vertexDataSphere[index].texcoord = {
				float(lonIndex) / float(kSubdivision),
				1.0f - float(latIndex) / float(kSubdivision)
			};
			vertexDataSphere[index].normal = {
				std::cosf(lat) * std::cosf(lon),
				std::sinf(lat),
				std::cosf(lat) * std::sinf(lon)
			};
		}
	}



#pragma endregion

#pragma region SphereIndex

	// インデックスリソース作成
	ComPtr<ID3D12Resource> indexResourceSphere = createBufferResouces(device.Get(), sizeof(uint32_t) * indexCount);

	// インデックスバッファビュー作成
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSphere{};
	indexBufferViewSphere.BufferLocation = indexResourceSphere->GetGPUVirtualAddress();
	indexBufferViewSphere.SizeInBytes = sizeof(uint32_t) * indexCount;
	indexBufferViewSphere.Format = DXGI_FORMAT_R32_UINT;

	// インデックスデータ書き込み
	uint32_t* indexDataSphere = nullptr;
	indexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSphere));

	uint32_t currentIndex = 0;
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			uint32_t a = latIndex * (kSubdivision + 1) + lonIndex;
			uint32_t b = (latIndex + 1) * (kSubdivision + 1) + lonIndex;
			uint32_t c = latIndex * (kSubdivision + 1) + (lonIndex + 1);
			uint32_t d = (latIndex + 1) * (kSubdivision + 1) + (lonIndex + 1);

			// 1枚目の三角形
			indexDataSphere[currentIndex++] = a;
			indexDataSphere[currentIndex++] = b;
			indexDataSphere[currentIndex++] = c;

			// 2枚目の三角形
			indexDataSphere[currentIndex++] = c;
			indexDataSphere[currentIndex++] = b;
			indexDataSphere[currentIndex++] = d;
		}
	}

	// TransformationMatrixリソース作成
	ComPtr<ID3D12Resource> transformationMatrixResourceSphere = createBufferResouces(device.Get(), sizeof(TransformationMatrix));
	TransformationMatrix* transformationMatrixDataSphere = nullptr;
	transformationMatrixResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSphere));
	transformationMatrixDataSphere->WVP = makeIdentity4x4();

	const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


#pragma endregion


#pragma region Camera // WVP行列用リソースの初期化

	// WVP（World × View × Projection）用の定数バッファを作成
	ComPtr<ID3D12Resource> wvpResouces = createBufferResouces(device.Get(), sizeof(TransformationMatrix));

	// 定数バッファにデータを書き込むためのポインタを取得
	TransformationMatrix* wvpData = nullptr;
	wvpResouces->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));

	// 初期値として単位行列をセット（描画前の安全な初期状態）
	wvpData->WVP = makeIdentity4x4();

#pragma endregion

#pragma region LightSprite // スプライト用マテリアルリソースの初期化

	// スプライトのマテリアル用の定数バッファを作成
	ComPtr<ID3D12Resource> materialResourcesSprite = createBufferResouces(device.Get(), sizeof(Material));

	// 書き込むアドレスを取得してポインタを保持
	Material* materialDataSprite = nullptr;
	materialResourcesSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));

	// 色を白に設定（RGBA全て1.0f）
	materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

	// スプライトはライティングを使わない
	materialDataSprite->enableLighting = false;

#pragma endregion

#pragma region LightSphere // 球体用マテリアルリソースの初期化

	ComPtr<ID3D12Resource> materialResourcesSphere = createBufferResouces(device.Get(), sizeof(Material));
	Material* materialDataSphere = nullptr;
	materialResourcesSphere->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSphere));

	// 白に設定
	materialDataSphere->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

	// 球体はライティングを有効にする
	materialDataSphere->enableLighting = true;

#pragma endregion

#pragma region LightOBJ // OBJモデル用マテリアルリソースの初期化

	ComPtr<ID3D12Resource> materialResourcesOBJ = createBufferResouces(device.Get(), sizeof(Material));
	Material* materialDataOBJ = nullptr;
	materialResourcesOBJ->Map(0, nullptr, reinterpret_cast<void**>(&materialDataOBJ));

	// 白に設定
	materialDataOBJ->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

	// OBJモデルはライティングを有効にする
	materialDataOBJ->enableLighting = true;

#pragma endregion

#pragma region SpriteTransform // スプライト用変換行列バッファ・ビューポートなどの初期化

	// スプライト描画用のWVP行列バッファを作成
	ComPtr<ID3D12Resource> transformationMatrixResourceSprite = createBufferResouces(device.Get(), sizeof(TransformationMatrix));
	TransformationMatrix* transformationMatrixDataSprite = nullptr;
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));

	// 単位行列で初期化
	transformationMatrixDataSprite->WVP = makeIdentity4x4();

	// ビューポート設定（画面全体）
	D3D12_VIEWPORT viewport{};
	viewport.Width = kClientWidth;
	viewport.Height = kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// シザー矩形（切り抜き範囲）も画面全体に設定
	D3D12_RECT scissorRect{};
	scissorRect.left = 0;
	scissorRect.right = kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;

	//=== ImGuiの初期化 ===//
	IMGUI_CHECKVERSION();
	ImGui::CreateContext(); // コンテキスト作成
	ImGui::StyleColorsDark(); // ダークテーマ使用
	ImGui_ImplWin32_Init(hwnd); // Win32プラットフォーム用初期化
	ImGui_ImplDX12_Init(
		device.Get(),                          // デバイス
		swapChainDesc.BufferCount,            // スワップチェインバッファ数
		rtvDesc.Format,                       // RTVフォーマット
		srvDescrriptorHeap.Get(),             // SRVヒープ
		srvDescrriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescrriptorHeap->GetGPUDescriptorHandleForHeapStart());


	//=== テクスチャ「uvChecker.png」の読み込みと転送 ===//
	DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

	// GPU側テクスチャリソースとアップロード用リソースを作成
	ComPtr<ID3D12Resource> textureResource = CreateTextureResource(device.Get(), metadata);
	ComPtr<ID3D12Resource> intermediateResource = UploadTextureData(textureResource.Get(), mipImages, device.Get(), commandList.Get());

	// ウィンドウサイズでDepthStencil用テクスチャを作成
	ComPtr<ID3D12Resource> depthStencilResource = CreateDepthStencilTextureResource(device.Get(), kClientWidth, kClientHeight);

	// SRVの設定（テクスチャをシェーダーに渡すビューを作成）
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	// SRVヒープのインデックス2に登録
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = GetCPUDescriptorHandle(srvDescrriptorHeap.Get(), descriptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = GetGPUDescriptorHandle(srvDescrriptorHeap.Get(), descriptorSizeSRV, 2);

	// SRVの作成
	device->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);

#pragma endregion

#pragma region MonsterBall // テクスチャ「monsterBall.png」の読み込みとSRV作成

	// モンスターボールの画像を読み込む
	DirectX::ScratchImage mipImages2 = LoadTexture("resources/monsterBall.png");
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();

	// テクスチャリソースを作成
	ComPtr<ID3D12Resource> textureResource2 = CreateTextureResource(device, metadata2);
	ComPtr<ID3D12Resource> intermediateResource2 = UploadTextureData(textureResource2, mipImages2, device, commandList);

	// SRVの設定（2Dテクスチャ）
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = metadata2.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	// SRVヒープのインデックス2（※複数同一インデックスで作成してる点に注意）
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescrriptorHeap, descriptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescrriptorHeap, descriptorSizeSRV, 2);

	// SRVを作成
	device->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);

#pragma endregion


#pragma region ModelData // OBJモデルの読み込みとバッファ設定

	// モデルデータ（バニー.obj）を読み込む
	ModelData modelData = LoadObjFile("resources", "bunny.obj");

	// 頂点数を取得して格納（頂点バッファ作成などで使用）
	uint32_t vertexCountObj = static_cast<uint32_t>(modelData.vertices.size());

	// 頂点用のリソース（バッファ）を作成
	ComPtr<ID3D12Resource> vertexResourceModel =
		createBufferResouces(device.Get(), sizeof(VertexData) * modelData.vertices.size());

	// 頂点バッファビュー（バッファの先頭アドレスやサイズなどを設定）
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewModel{};
	vertexBufferViewModel.BufferLocation = vertexResourceModel->GetGPUVirtualAddress(); // GPU仮想アドレスの取得
	vertexBufferViewModel.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size()); // バッファ全体のサイズ
	vertexBufferViewModel.StrideInBytes = sizeof(VertexData); // 頂点1つ分のサイズ

	// 頂点リソースへCPUからデータを書き込む
	VertexData* vertexDataModel = nullptr;
	vertexResourceModel->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataModel));
	std::memcpy(vertexDataModel, modelData.vertices.data(), sizeof(VertexData)* vertexCountObj);

	// OBJモデル用のWVP行列バッファ（Transform用定数バッファ）を作成
	ComPtr<ID3D12Resource> transformationMatrixResourceOBJ =
		createBufferResouces(device.Get(), sizeof(TransformationMatrix));
	TransformationMatrix* transformationMatrixDataOBJ = nullptr;
	transformationMatrixResourceOBJ->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataOBJ));
	transformationMatrixDataOBJ->WVP = makeIdentity4x4(); // 初期値として単位行列を設定

#pragma endregion

#pragma region model Texture // OBJモデル用テクスチャの読み込みとSRV作成

	// モデルのマテリアルに指定されたテクスチャを読み込む
	DirectX::ScratchImage mipImages3 = LoadTexture(modelData.material.textureFilePath);
	const DirectX::TexMetadata& metadata3 = mipImages3.GetMetadata();

	// GPU上のテクスチャリソースとアップロード用リソースを作成
	ComPtr<ID3D12Resource> textureResource3 = CreateTextureResource(device.Get(), metadata3);
	ComPtr<ID3D12Resource> intermediateResource3 =
		UploadTextureData(textureResource3.Get(), mipImages3, device.Get(), commandList.Get());

	// SRV（シェーダーリソースビュー）の設定（テクスチャをシェーダーで使用するためのビュー）
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc3{};
	srvDesc3.Format = metadata3.format;
	srvDesc3.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc3.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc3.Texture2D.MipLevels = UINT(metadata3.mipLevels);

	// ディスクリプタヒープ内の3番目のスロットにSRVを作成（バニー.obj用）
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU3 = GetCPUDescriptorHandle(srvDescrriptorHeap.Get(), descriptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU3 = GetGPUDescriptorHandle(srvDescrriptorHeap.Get(), descriptorSizeSRV, 2);

	device->CreateShaderResourceView(textureResource3.Get(), &srvDesc3, textureSrvHandleCPU3);

#pragma endregion

	//=======================================//
	//=== DSV（深度ステンシルビュー）設定 ===//
	//=======================================//
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度 + ステンシルの一般的なフォーマット
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2Dテクスチャとして扱う

	// DSVヒープの先頭スロットにDSVを作成
	device->CreateDepthStencilView(
		depthStencilResource.Get(),
		&dsvDesc,
		dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

#pragma region LightDirectionSprite // スプライト用ライト設定

	// スプライト用の平行光源（DirectionalLight）バッファを作成
	ComPtr<ID3D12Resource> materialResourceDirectionSprite =
		createBufferResouces(device.Get(), sizeof(DirectionalLight));
	DirectionalLight* directionalLightDataSprite = nullptr;
	materialResourceDirectionSprite->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightDataSprite));

	// 光の色・方向・強さを設定
	directionalLightDataSprite->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightDataSprite->direction = { 0.0f,-1.0f,0.0f }; // 上から下方向
	directionalLightDataSprite->intensity = 1.0f;

#pragma endregion

#pragma region LightDirectionSphere // 球体用ライト設定

	ComPtr<ID3D12Resource> materialResourceDirectionSphere =
		createBufferResouces(device.Get(), sizeof(DirectionalLight));
	DirectionalLight* directionalLightDataSphere = nullptr;
	materialResourceDirectionSphere->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightDataSphere));

	directionalLightDataSphere->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightDataSphere->direction = { 0.0f,-1.0f,0.0f };
	directionalLightDataSphere->intensity = 1.0f;

#pragma endregion

#pragma region LightDirectionOBJ // OBJモデル用ライト設定

	ComPtr<ID3D12Resource> materialResourceDirectionOBJ =
		createBufferResouces(device.Get(), sizeof(DirectionalLight));
	DirectionalLight* directionalLightDataOBJ = nullptr;
	materialResourceDirectionOBJ->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightDataOBJ));

	directionalLightDataOBJ->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightDataOBJ->direction = { 0.0f,-1.0f,0.0f };
	directionalLightDataOBJ->intensity = 1.0f;

#pragma endregion

#pragma region UVTransform // 各描画対象のUV変換行列を初期化

	materialDataSprite->uvTransform = makeIdentity4x4();
	materialDataSphere->uvTransform = makeIdentity4x4();
	materialDataOBJ->uvTransform = makeIdentity4x4();

#pragma endregion

	//================================================================//
	//=== 各カメラTransform（カメラの位置・回転・スケール）初期化 ===//
	//===============================================================//
	Transform cameraTransformSprite = {
		{1.0f, 1.0f, 1.0f},    // Scale
		{0.0f, 0.0f, 0.0f},    // Rotate
		{0.0f, 0.0f, -10.0f}   // Translate（カメラをZ方向に引く）
	};

	Transform cameraTransformSphere = {
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, -10.0f}
	};

	Transform cameraTransformOBJ = {
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, -10.0f}
	};

	//===========================================================//
	//=== 各描画対象のTransform（位置・回転・スケール）初期化 ===//
	//===========================================================//


	Transform transformSprite = {
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f}
	};

	Transform transformSphere = {
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f}
	};

	Transform transformOBJ = {
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f}
	};

	//=======================================================================//
	//=== スプライト用のUV変換行列（テクスチャUVの回転・スケーリングなど）===//
	//=======================================================================//

	Transform uvTransformSprite = {
		{1.0f, 1.0f, 1.0f},    // スケール
		{0.0f, 1.0f, 0.0f},    // 回転（ここは疑似的な使用？）
		{0.0f, 0.0f, 0.0f}     // 平行移動
	};

	//==============================//
	//=== サウンド読み込みと再生 ===//
	//==============================//

	SoundData soundData1 = SoundLoadWave("resources/Alarm01.wav"); // WAVファイル読み込み
	SoundPlayWave(xAudio2.Get(), soundData1);                      // サウンドを再生


	//==============================//
	//=== 各描画対象の有効フラグ ===//
	//==============================//

	bool isSphereActive = true;  // 球体を描画するか
	bool isSpriteActive = true;  // スプライトを描画するか
	bool isOBJActive = false;    // OBJモデルを描画するか（初期状態ではOFF）


	//============メインループ==========//

	//windowの×ボタンが押されるまでループ
	while (msg.message != WM_QUIT)
	{





		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else
		{

#pragma region グラフィックスコマンド

			BYTE key[256] = {};
			HRESULT result = keyboard->GetDeviceState(sizeof(key), key);
			if (FAILED(result)) {
				// 再Acquireしてリトライ
				keyboard->Acquire();
				result = keyboard->GetDeviceState(sizeof(key), key);
				if (FAILED(result)) {
					OutputDebugStringA("キー状態の取得に失敗しました\n");
				}
			}

			// キー判定（成功した場合のみ）
			if (SUCCEEDED(result)) {
				if (key[DIK_SPACE] & 0x80) {
					OutputDebugStringA("Hit 0\n");
				}
			}

			/*	memcpy(prevKey, key, sizeof(key));*/

#pragma endregion                                     

			// マテリアルのライティング有効状態を一時変数に保存（チェックボックス表示などに使用）
			bool temp_enableLightingSprite = (materialDataSprite->enableLighting != 0);
			bool temp_enableLightingSphere = (materialDataSphere->enableLighting != 0);
			bool temp_enableLightingOBJ = (materialDataOBJ->enableLighting != 0);

			//=== ImGui 新フレーム開始 ===//
			ImGui_ImplDX12_NewFrame();     // DX12用ImGui新フレーム
			ImGui_ImplWin32_NewFrame();    // Win32用ImGui新フレーム
			ImGui::NewFrame();             // ImGuiの描画準備
			ImGui::ShowDemoWindow();       // デモ用ウィンドウを表示（デバッグ・確認用）

			//=== ライトの方向ベクトルを正規化（方向ベクトルは常に正規化されている必要がある）===//
			directionalLightDataSprite->direction = Normalize(directionalLightDataSprite->direction);
			directionalLightDataSphere->direction = Normalize(directionalLightDataSphere->direction);
			directionalLightDataOBJ->direction = Normalize(directionalLightDataOBJ->direction);

			//=== 球体の回転（デバッグ用）===//
			// transform.rotate.y += 0.03f;

#pragma region CameraSprite // スプライト用のカメラとWVP行列設定

// スプライトのワールド行列作成（スケール・回転・位置）
			Matrix4x4 worldMatrixSprite = MakeAffineMatrix(
				transformSprite.scale,
				transformSprite.rotate,
				transformSprite.translate);

			// カメラ行列（スプライトは基本的に2D扱いなので使わないが形式上用意）
			Matrix4x4 cameraMatrixSprite = MakeAffineMatrix(
				cameraTransformSprite.scale,
				cameraTransformSprite.rotate,
				cameraTransformSprite.translate);

			// ビュー行列（今回は2D描画のため単位行列）
			Matrix4x4 viewMatrixSprite = makeIdentity4x4();

			// 正射影行列（2Dスプライト描画用）
			Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(
				0.0f, 0.0f,
				float(kClientWidth),
				float(kClientHeight),
				0.0f, 100.0f);

			// WVP行列（ワールド × ビュー × プロジェクション）
			Matrix4x4 worldViewProjectionMatrixSprite = Multiply(
				worldMatrixSprite,
				Multiply(viewMatrixSprite, projectionMatrixSprite));

			// スプライト用定数バッファに設定
			transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;
			transformationMatrixDataSprite->World = worldMatrixSprite;

#pragma endregion

#pragma region CameraSphere // 球体用のカメラとWVP行列設定

			// 球体のワールド行列作成
			Matrix4x4 worldMatrixSphere = MakeAffineMatrix(
				transformSphere.scale,
				transformSphere.rotate,
				transformSphere.translate);

			// カメラ行列作成（球体は3Dなのでビュー行列のために逆行列を使う）
			Matrix4x4 cameraMatrixSphere = MakeAffineMatrix(
				cameraTransformSphere.scale,
				cameraTransformSphere.rotate,
				cameraTransformSphere.translate);

			// ビュー行列（カメラの逆行列）
			Matrix4x4 viewMatrixSphere = Inverse(cameraMatrixSphere);

			// 透視投影行列（3Dの見え方に遠近感をつける）
			Matrix4x4 projectionMatrixSphere = MakePerspectiveFovMatrix(
				0.45f,                              // 視野角（FOV）
				float(kClientWidth) / float(kClientHeight), // アスペクト比
				0.1f, 100.0f);                      // 近クリップ面・遠クリップ面

			// WVP行列
			Matrix4x4 worldViewProjectionMatrixSphere = Multiply(
				worldMatrixSphere,
				Multiply(viewMatrixSphere, projectionMatrixSphere));

			// 球体用定数バッファに設定
			transformationMatrixDataSphere->WVP = worldViewProjectionMatrixSphere;
			transformationMatrixDataSphere->World = worldMatrixSphere;

#pragma endregion

#pragma region CameraOBJ // OBJモデル用のカメラとWVP行列設定

			// OBJモデルのワールド行列作成
			Matrix4x4 worldMatrixOBJ = MakeAffineMatrix(
				transformOBJ.scale,
				transformOBJ.rotate,
				transformOBJ.translate);

			// カメラ行列作成
			Matrix4x4 cameraMatrixOBJ = MakeAffineMatrix(
				cameraTransformOBJ.scale,
				cameraTransformOBJ.rotate,
				cameraTransformOBJ.translate);

			// ビュー行列（カメラの逆行列）
			Matrix4x4 viewMatrixOBJ = Inverse(cameraMatrixOBJ);

			// 透視投影行列
			Matrix4x4 projectionMatrixOBJ = MakePerspectiveFovMatrix(
				0.45f,
				float(kClientWidth) / float(kClientHeight),
				0.1f, 100.0f);

			// WVP行列
			Matrix4x4 worldViewProjectionMatrixOBJ = Multiply(
				worldMatrixOBJ,
				Multiply(viewMatrixOBJ, projectionMatrixOBJ));

			// OBJ用定数バッファに設定
			transformationMatrixDataOBJ->WVP = worldViewProjectionMatrixOBJ;
			transformationMatrixDataOBJ->World = worldMatrixOBJ;

#pragma endregion


#pragma region ▼ ImGui入力処理 ▼

			//=== 描画対象の選択 ===//
			ImGui::Begin("Control Panel");
			static const char* drawTargetNames[] = { "Sphere", "Sprite", "OBJ" };
			static int drawTargetIndex = 0; // 0: Sphere, 1: Sprite, 2: OBJ
			ImGui::Combo("Draw Target", &drawTargetIndex, drawTargetNames, IM_ARRAYSIZE(drawTargetNames));
			ImGui::Checkbox("Use Monster Ball", &useMonsterBall);
			ImGui::End();

			//=== Transform ===//
			ImGui::Begin("Transform Settings");
			switch (drawTargetIndex) {
			case 0:
				ImGui::Text("Sphere Transform");
				ImGui::DragFloat3("Position", &transformSphere.translate.x, 0.1f);
				ImGui::DragFloat3("Rotation", &transformSphere.rotate.x, 0.1f);
				ImGui::DragFloat3("Scale", &transformSphere.scale.x, 0.1f, 0.0f, 10.0f);
				break;
			case 1:
				ImGui::Text("Sprite Transform");
				ImGui::DragFloat3("Position", &transformSprite.translate.x, 0.1f);
				ImGui::DragFloat3("Rotation", &transformSprite.rotate.x, 0.1f);
				ImGui::DragFloat3("Scale", &transformSprite.scale.x, 0.1f, 0.0f, 10.0f);
				break;
			case 2:
				ImGui::Text("OBJ Transform");
				ImGui::DragFloat3("Position", &transformOBJ.translate.x, 0.1f);
				ImGui::DragFloat3("Rotation", &transformOBJ.rotate.x, 0.1f);
				ImGui::DragFloat3("Scale", &transformOBJ.scale.x, 0.1f, 0.0f, 10.0f);
				break;
			}
			ImGui::End();

			//=== Material & Light ===//
			ImGui::Begin("Material & Lighting");
			switch (drawTargetIndex) {
			case 0:
				ImGui::Text("Sphere Material");
				ImGui::ColorEdit4("Color", &materialDataSphere->color.x);
				ImGui::Checkbox("Enable Lighting", &temp_enableLightingSphere);
				materialDataSphere->enableLighting = temp_enableLightingSphere ? 1 : 0;

				ImGui::Text("Sphere Light");
				ImGui::ColorEdit3("Light Color", &directionalLightDataSphere->color.x);
				ImGui::SliderFloat3("Direction", &directionalLightDataSphere->direction.x, -1.0f, 1.0f);
				ImGui::DragFloat("Intensity", &directionalLightDataSphere->intensity);
				break;

			case 1:
				ImGui::Text("Sprite Material");
				ImGui::ColorEdit4("Color", &materialDataSprite->color.x);
				ImGui::Checkbox("Enable Lighting", &temp_enableLightingSprite);
				materialDataSprite->enableLighting = temp_enableLightingSprite ? 1 : 0;

				ImGui::Text("Sprite Light");
				ImGui::ColorEdit3("Light Color", &directionalLightDataSprite->color.x);
				ImGui::SliderFloat3("Direction", &directionalLightDataSprite->direction.x, -1.0f, 1.0f);
				ImGui::DragFloat("Intensity", &directionalLightDataSprite->intensity);
				break;

			case 2:
				ImGui::Text("OBJ Material");
				ImGui::ColorEdit4("Color", &materialDataOBJ->color.x);
				ImGui::Checkbox("Enable Lighting", &temp_enableLightingOBJ);
				materialDataOBJ->enableLighting = temp_enableLightingOBJ ? 1 : 0;

				ImGui::Text("OBJ Light");
				ImGui::ColorEdit3("Light Color", &directionalLightDataOBJ->color.x);
				ImGui::SliderFloat3("Direction", &directionalLightDataOBJ->direction.x, -1.0f, 1.0f);
				ImGui::DragFloat("Intensity", &directionalLightDataOBJ->intensity);
				break;
			}
			ImGui::End();

			//=== UV Transform (Sprite専用) ===//
			if (drawTargetIndex == 1) {
				ImGui::Begin("Sprite UV Transform");
				ImGui::DragFloat2("Translate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
				ImGui::DragFloat2("Scale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
				ImGui::SliderAngle("Rotate", &uvTransformSprite.rotate.z);
				ImGui::End();
			}

#pragma endregion ▲ ImGui入力処理 終了 ▲

#pragma region ▼ 描画準備処理 ▼

			//--- UVTransformの行列計算 ---
			Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
			uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
			materialDataSprite->uvTransform = uvTransformMatrix;
			materialDataSphere->uvTransform = makeIdentity4x4(); // Sphereは無効

			//--- バックバッファの取得 ---
			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

			//--- リソースバリア（Present → RenderTarget）---
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = swapChainResouces[backBufferIndex].Get();
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			commandList->ResourceBarrier(1, &barrier);

			//--- 描画ターゲット・クリア ---
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);

			float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
			commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);




			ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescrriptorHeap.Get() };
			commandList->SetDescriptorHeaps(1, descriptorHeaps);

			//コマンドを積む
			commandList->RSSetViewports(1, &viewport);
			commandList->RSSetScissorRects(1, &scissorRect);




			//=== 描画処理 ===//
			switch (drawTargetIndex) {
			case 0: // Sphere
				commandList->SetGraphicsRootSignature(rootsignatrue.Get());
				commandList->SetPipelineState(graphicsPipelineState.Get());
				commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSphere);
				commandList->IASetIndexBuffer(&indexBufferViewSphere);
				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandList->SetGraphicsRootConstantBufferView(0, materialResourcesSphere->GetGPUVirtualAddress());
				commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSphere->GetGPUVirtualAddress());
				commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
				commandList->SetGraphicsRootConstantBufferView(3, materialResourceDirectionSphere->GetGPUVirtualAddress());
				commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
				break;

			case 1: // Sprite
				commandList->SetGraphicsRootSignature(rootsignatrue.Get());
				commandList->SetPipelineState(graphicsPipelineState.Get());
				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
				commandList->IASetIndexBuffer(&indexBufferViewSprite);
				commandList->SetGraphicsRootConstantBufferView(0, materialResourcesSprite->GetGPUVirtualAddress());
				commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
				commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
				commandList->SetGraphicsRootConstantBufferView(3, materialResourceDirectionSprite->GetGPUVirtualAddress());
				commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
				break;

			case 2: // OBJ
				commandList->SetGraphicsRootSignature(rootsignatrue.Get());
				commandList->SetPipelineState(graphicsPipelineState.Get());
				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandList->IASetVertexBuffers(0, 1, &vertexBufferViewModel);
				commandList->SetGraphicsRootConstantBufferView(0, materialResourcesOBJ->GetGPUVirtualAddress());
				commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceOBJ->GetGPUVirtualAddress());
				commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
				commandList->SetGraphicsRootConstantBufferView(3, materialResourceDirectionOBJ->GetGPUVirtualAddress());
				commandList->DrawInstanced(vertexCountObj, 1, 0, 0);
				break;
			}




			ImGui::Render();



			//実際のcommmandList残り時間imguiの描画コマンドを積む
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

			//renderTarGetからPresentにする
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

			//transSitiionBarrierを張る
			commandList->ResourceBarrier(1, &barrier);

			hr = commandList->Close();
			assert(SUCCEEDED(hr));

			ID3D12CommandList* commandLists[] = { commandList.Get() };
			commandQueue->ExecuteCommandLists(1, commandLists);
			swapChain->Present(1, 0);


			//Fenceの更新
			fenceValue++;

			//GPUがそこまでたどり着いた時に Fenceの値を指定した値に代入するようにsignalを送る
			commandQueue->Signal(fence.Get(), fenceValue);

			if (fence->GetCompletedValue() < fenceValue)
			{
				//指定したsignal似たとりついていないのでたどり着くまでに待つようにイベントを指定する
				fence->SetEventOnCompletion(fenceValue, fenceEvent);
				//イベントを待つ
				WaitForSingleObject(fenceEvent, INFINITE);
			}


			hr = commandAllocator->Reset();
			assert(SUCCEEDED(hr));
			hr = commandList->Reset(commandAllocator.Get(), nullptr);
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

	//xAudio2解放
	xAudio2.Reset();

	//音声データ解放
	SoundUnLoad(&soundData1);

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	//comの終了時
	CoUninitialize();

	return 0;
}
