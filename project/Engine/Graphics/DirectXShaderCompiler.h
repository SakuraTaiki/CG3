#pragma once

// DXCが使用するWindows型とSAL注釈を先に定義する
#include <Windows.h>
#include <dxcapi.h>
#include <string>
#include <wrl.h>

#pragma comment(lib, "dxcompiler.lib")

// HLSL のコンパイルだけを担当するクラス。
// DirectXCommon から DXC 関連の処理を分離する。
class DirectXShaderCompiler {
public:
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    void Initialize();

    ComPtr<IDxcBlob> Compile(
        const std::wstring& filePath,
        const wchar_t* profile
    );

private:
    // DXC 用オブジェクト。
    ComPtr<IDxcUtils> dxcUtils_;
    ComPtr<IDxcCompiler3> dxcCompiler_;
    ComPtr<IDxcIncludeHandler> includeHandler_;
};