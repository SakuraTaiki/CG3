#include "DirectXShaderCompiler.h"

#include <cassert>

void DirectXShaderCompiler::Initialize() {
    HRESULT hr = DxcCreateInstance(
        CLSID_DxcUtils,
        IID_PPV_ARGS(&dxcUtils_)
    );
    assert(SUCCEEDED(hr));

    hr = DxcCreateInstance(
        CLSID_DxcCompiler,
        IID_PPV_ARGS(&dxcCompiler_)
    );
    assert(SUCCEEDED(hr));

    hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
    assert(SUCCEEDED(hr));
}

DirectXShaderCompiler::ComPtr<IDxcBlob> DirectXShaderCompiler::Compile(
    const std::wstring& filePath,
    const wchar_t* profile
) {
    OutputDebugStringW(L"----------------------------------------\n");
    OutputDebugStringW(L"Begin CompileShader: ");
    OutputDebugStringW(filePath.c_str());
    OutputDebugStringW(L"\n");

    // HLSL ファイルを読み込む。
    ComPtr<IDxcBlobEncoding> shaderSource = nullptr;

    HRESULT hr = dxcUtils_->LoadFile(
        filePath.c_str(),
        nullptr,
        &shaderSource
    );

    if (FAILED(hr)) {
        OutputDebugStringA("ERROR: Failed to load shader file.\n");
        OutputDebugStringW(filePath.c_str());
        OutputDebugStringA("\n----------------------------------------\n");

        assert(false && "Shader File Not Found!");
        return nullptr;
    }

    DxcBuffer shaderSourceBuffer{};
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8;

    // main 関数を指定して、指定 profile でコンパイルする。
    LPCWSTR arguments[] = {
        filePath.c_str(),
        L"-E", L"main",
        L"-T", profile,
        L"-Zi", L"-Qembed_debug",
        L"-Od",
        L"-Zpr",
    };

    ComPtr<IDxcResult> shaderResult = nullptr;

    hr = dxcCompiler_->Compile(
        &shaderSourceBuffer,
        arguments,
        _countof(arguments),
        includeHandler_.Get(),
        IID_PPV_ARGS(&shaderResult)
    );

    if (FAILED(hr)) {
        OutputDebugStringA("ERROR: DxcCompiler::Compile failed.\n");
        assert(false);
        return nullptr;
    }

    // コンパイルエラーがあれば表示して止める。
    ComPtr<IDxcBlobUtf8> shaderError = nullptr;

    shaderResult->GetOutput(
        DXC_OUT_ERRORS,
        IID_PPV_ARGS(&shaderError),
        nullptr
    );

    if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
        std::string errorMsg = shaderError->GetStringPointer();

        OutputDebugStringA("----------------------------------------\n");
        OutputDebugStringA("HLSL Compile Error:\n");
        OutputDebugStringA(errorMsg.c_str());
        OutputDebugStringA("----------------------------------------\n");

        assert(false && "Shader Compile Error");
    }

    ComPtr<IDxcBlob> shaderBlob = nullptr;

    hr = shaderResult->GetOutput(
        DXC_OUT_OBJECT,
        IID_PPV_ARGS(&shaderBlob),
        nullptr
    );
    assert(SUCCEEDED(hr));

    OutputDebugStringA("CompileShader Success!\n");
    OutputDebugStringA("----------------------------------------\n");

    return shaderBlob;
}