#include "DirectXCommon.h"
#include "WinApp.h"
#include <vector>
#include <cassert>
#include <format>
#include <thread>
#include "MyMath.h"
#include <algorithm>

using namespace Microsoft::WRL;

void DirectXCommon::Initialize(WinApp* winApp) {
    assert(winApp);

    winApp_ =
        winApp;

    width_ =
        static_cast<uint32_t>(
            (std::max)(
                winApp_->GetWidth(),
                1
                )
            );

    height_ =
        static_cast<uint32_t>(
            (std::max)(
                winApp_->GetHeight(),
                1
                )
            );

    fpsLimiter_.Initialize();

    InitializeDevice();
    InitializeCommand();
    InitializeSwapChain();
    InitializeRenderTargetView();
    InitializeDepthStencilView();
    InitializeFence();

    shaderCompiler_.Initialize();

    InitializeRenderTexture();
    InitializeCopyImagePipeline();

    // 初期ウィンドウ生成時のWM_SIZEを消費
    uint32_t ignoredWidth = 0;
    uint32_t ignoredHeight = 0;

    winApp_->ConsumeResize(
        ignoredWidth,
        ignoredHeight
    );

}
void DirectXCommon::PreDrawForRenderTexture()
{

    ResizeIfNeeded();

    TransitionResource(
        renderTextureResource_.Get(),
        renderTextureState_,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    renderTextureState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
        dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    commandList_->OMSetRenderTargets(1, &renderTextureRtvHandle_, false, &dsvHandle);

    float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
    commandList_->ClearRenderTargetView(renderTextureRtvHandle_, clearColor, 0, nullptr);
    commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    D3D12_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(width_);
    viewport.Height = static_cast<float>(height_);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    commandList_->RSSetViewports(1, &viewport);

    D3D12_RECT scissorRect{};
    scissorRect.right = width_;
    scissorRect.bottom = height_;
    commandList_->RSSetScissorRects(1, &scissorRect);


}


void DirectXCommon::DrawRenderTextureToSwapChain()
{
    //==================================================
    // RenderTextureをPixelShaderから読める状態へ変更
    //==================================================

    TransitionResource(
        renderTextureResource_.Get(),
        renderTextureState_,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    renderTextureState_ =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    // Outlineが有効な場合はDepthもPixelShaderから読み込む
    if (outlineSettings_.enabled) {
        TransitionResource(
            depthStencilResource_.Get(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
    }

    //==================================================
    // DescriptorHeap設定
    //==================================================

    ID3D12DescriptorHeap* heaps[] = {
        renderTextureSrvHeap_.Get()
    };

    commandList_->SetDescriptorHeaps(
        1,
        heaps
    );

    //==================================================
    // RootSignature・PSO設定
    //==================================================

    commandList_->SetGraphicsRootSignature(
        copyImageRootSignature_.Get()
    );

    if (outlineSettings_.enabled) {
        commandList_->SetPipelineState(
            outlinePipelineState_.Get()
        );
    } else if (gaussianSettings_.enabled) {
        commandList_->SetPipelineState(
            gaussianPipelineState_.Get()
        );
    } else if (smoothingSettings_.enabled) {
        commandList_->SetPipelineState(
            smoothingPipelineState_.Get()
        );
    } else {
        commandList_->SetPipelineState(
            postEffectPipelineState_.Get()
        );
    }

    commandList_->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );

    // t0 = RenderTexture
    // t1 = DepthTexture
    //
    // 2つのSRVが同じDescriptorHeap内で連続しているため、
    // 先頭のGPUハンドルを指定すれば両方参照できる
    commandList_->SetGraphicsRootDescriptorTable(
        0,
        renderTextureSrvHandleGPU_
    );

    //==================================================
    // Outline
    //==================================================

    if (outlineSettings_.enabled) {
        struct OutlineConstants {
            float outlineColor[4];

            float outlineThreshold;
            float outlineStrength;
            float nearClip;
            float farClip;

            uint32_t outlineThickness;
            uint32_t enableOutline;
            float padding[2];
        };

        OutlineConstants constants{};

        constants.outlineColor[0] =
            outlineSettings_.color[0];

        constants.outlineColor[1] =
            outlineSettings_.color[1];

        constants.outlineColor[2] =
            outlineSettings_.color[2];

        constants.outlineColor[3] =
            outlineSettings_.color[3];

        constants.outlineThreshold =
            (std::max)(
                outlineSettings_.threshold,
                0.00001f
                );

        constants.outlineStrength =
            (std::max)(
                outlineSettings_.strength,
                0.0f
                );

        constants.nearClip =
            (std::max)(
                outlineSettings_.nearClip,
                0.00001f
                );

        constants.farClip =
            (std::max)(
                outlineSettings_.farClip,
                constants.nearClip + 0.00001f
                );

        constants.outlineThickness =
            static_cast<uint32_t>(
                std::clamp(
                    outlineSettings_.thickness,
                    1,
                    4
                )
                );

        constants.enableOutline = 1;

        commandList_->SetGraphicsRoot32BitConstants(
            1,
            12,
            &constants,
            0
        );
    }

    //==================================================
    // GaussianFilter
    //==================================================

    else if (gaussianSettings_.enabled) {
        struct GaussianConstants {
            uint32_t blurRadius;
            float sigma;
            float blurStrength;
            float padding;
        };

        GaussianConstants constants{};

        constants.blurRadius =
            static_cast<uint32_t>(
                std::clamp(
                    gaussianSettings_.radius,
                    1,
                    4
                )
                );

        constants.sigma =
            std::clamp(
                gaussianSettings_.sigma,
                0.1f,
                10.0f
            );

        constants.blurStrength =
            std::clamp(
                gaussianSettings_.strength,
                0.0f,
                1.0f
            );

        commandList_->SetGraphicsRoot32BitConstants(
            1,
            4,
            &constants,
            0
        );
    }

    //==================================================
    // BoxFilter
    //==================================================

    else if (smoothingSettings_.enabled) {
        struct SmoothingConstants {
            uint32_t blurRadius;
            float blurStrength;
            float padding[2];
        };

        SmoothingConstants constants{};

        constants.blurRadius =
            static_cast<uint32_t>(
                std::clamp(
                    smoothingSettings_.radius,
                    1,
                    4
                )
                );

        constants.blurStrength =
            std::clamp(
                smoothingSettings_.strength,
                0.0f,
                1.0f
            );

        commandList_->SetGraphicsRoot32BitConstants(
            1,
            4,
            &constants,
            0
        );
    }

    //==================================================
    // Vignette・Grayscale
    //==================================================

    else {
        struct PostEffectConstants {
            float vignetteIntensity;
            float vignetteRadius;
            float vignetteSoftness;
            float aspectRatio;

            uint32_t enableVignette;
            uint32_t enableGrayScale;
            uint32_t padding[2];
        };

        PostEffectConstants constants{};

        constants.vignetteIntensity =
            vignetteSettings_.intensity;

        constants.vignetteRadius =
            vignetteSettings_.radius;

        constants.vignetteSoftness =
            vignetteSettings_.softness;

        constants.aspectRatio =
            static_cast<float>(width_) /
            static_cast<float>(height_);

        constants.enableVignette =
            vignetteSettings_.enabled
            ? 1u
            : 0u;

        constants.enableGrayScale =
            enableGrayScale_
            ? 1u
            : 0u;

        commandList_->SetGraphicsRoot32BitConstants(
            1,
            8,
            &constants,
            0
        );
    }

    //==================================================
    // Fullscreen Triangle描画
    //==================================================

    commandList_->DrawInstanced(
        3,
        1,
        0,
        0
    );

    //==================================================
    // Depthを次フレームの書き込み状態へ戻す
    //==================================================

    if (outlineSettings_.enabled) {
        TransitionResource(
            depthStencilResource_.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_DEPTH_WRITE
        );
    }
}


void DirectXCommon::InitializeDevice() {
    // DXGIファクトリーの生成
#ifdef _DEBUG
    ComPtr<ID3D12Debug1> debugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        debugController->SetEnableGPUBasedValidation(TRUE);
    }
#endif

    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
    assert(SUCCEEDED(hr));

    // アダプターの列挙とデバイス生成
    ComPtr<IDXGIAdapter4> useAdapter = nullptr;
    for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC3 adapterDesc{};
        hr = useAdapter->GetDesc3(&adapterDesc);
        if (FAILED(hr)) continue;

        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
            // D3D12デバイス生成を試みる
            hr = D3D12CreateDevice(useAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&device_));
            if (SUCCEEDED(hr)) {
                break;
            }
        }
        useAdapter = nullptr;
    }
    assert(device_ != nullptr);

#ifdef _DEBUG
    // デバッグ時のエラー停止設定
    ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
    if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        // infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true); 
    }
#endif
}

void DirectXCommon::InitializeCommand() {
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    HRESULT hr = device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_));
    assert(SUCCEEDED(hr));

    hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
    assert(SUCCEEDED(hr));

    hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
    assert(SUCCEEDED(hr));
}

void DirectXCommon::InitializeSwapChain() {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = width_;
    swapChainDesc.Height = height_;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    HRESULT hr = dxgiFactory_->CreateSwapChainForHwnd(
        commandQueue_.Get(),
        winApp_->GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
    assert(SUCCEEDED(hr));
}

void DirectXCommon::InitializeRenderTargetView() {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = 3;
    HRESULT hr = device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap_));
    assert(SUCCEEDED(hr));

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    UINT handleSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    for (UINT i = 0; i < 2; ++i) {
        hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
        assert(SUCCEEDED(hr));
        device_->CreateRenderTargetView(swapChainResources_[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += handleSize;
    }
}

void DirectXCommon::InitializeDepthStencilView() {
    // DSV用ヒープ生成
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HRESULT hr = device_->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvDescriptorHeap_));
    assert(SUCCEEDED(hr));

    // 深度リソース生成
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = width_;
    resourceDesc.Height = height_;
    resourceDesc.MipLevels = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clearValue.DepthStencil.Depth = 1.0f;

    hr = device_->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&depthStencilResource_));
    assert(SUCCEEDED(hr));

    
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format =
        DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension =
        D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags =
        D3D12_DSV_FLAG_NONE;

    device_->CreateDepthStencilView(
        depthStencilResource_.Get(),
        &dsvDesc,
        dsvDescriptorHeap_
        ->GetCPUDescriptorHandleForHeapStart()
    );

}

void DirectXCommon::InitializeFence() {
    HRESULT hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    assert(SUCCEEDED(hr));
    fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(fenceEvent_ != nullptr);
}


DirectXCommon::ComPtr<IDxcBlob> DirectXCommon::CompileShader(
    const std::wstring& filePath,
    const wchar_t* profile
) {
    return shaderCompiler_.Compile(filePath, profile);
}


void DirectXCommon::PreDraw()
{
    // 現在表示対象になっているBackBufferを取得
    UINT backBufferIndex =
        swapChain_->GetCurrentBackBufferIndex();

    //==================================================
    // BackBufferを描画可能な状態へ変更
    //==================================================

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type =
        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags =
        D3D12_RESOURCE_BARRIER_FLAG_NONE;

    barrier.Transition.pResource =
        swapChainResources_[backBufferIndex].Get();

    barrier.Transition.StateBefore =
        D3D12_RESOURCE_STATE_PRESENT;

    barrier.Transition.StateAfter =
        D3D12_RESOURCE_STATE_RENDER_TARGET;

    barrier.Transition.Subresource =
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList_->ResourceBarrier(
        1,
        &barrier
    );

    //==================================================
    // 現在のBackBufferに対応するRTVを取得
    //==================================================

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        rtvDescriptorHeap_
        ->GetCPUDescriptorHandleForHeapStart();

    UINT rtvDescriptorSize =
        device_->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV
        );

    rtvHandle.ptr +=
        static_cast<SIZE_T>(
            backBufferIndex
            ) *
        static_cast<SIZE_T>(
            rtvDescriptorSize
            );

    //==================================================
    // BackBufferだけをRenderTargetとして設定
    //==================================================

    // DepthStencilはここでは設定しない。
    //
    // PreDrawForRenderTexture()で作成された深度情報を
    // DrawRenderTextureToSwapChain()のOutline処理で使用するため。
    commandList_->OMSetRenderTargets(
        1,
        &rtvHandle,
        false,
        nullptr
    );

    //==================================================
    // BackBufferをクリア
    //==================================================

    const float clearColor[] = {
        0.1f,
        0.25f,
        0.5f,
        1.0f
    };

    commandList_->ClearRenderTargetView(
        rtvHandle,
        clearColor,
        0,
        nullptr
    );

    // ここでは深度バッファをクリアしない。
    // 深度のクリアはPreDrawForRenderTexture()で行う。

    //==================================================
    // Viewport設定
    //==================================================

    D3D12_VIEWPORT viewport{};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;

    viewport.Width =
        static_cast<float>(width_);

    viewport.Height =
        static_cast<float>(height_);

    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    commandList_->RSSetViewports(
        1,
        &viewport
    );

    //==================================================
    // ScissorRect設定
    //==================================================

    D3D12_RECT scissorRect{};
    scissorRect.left = 0;
    scissorRect.top = 0;

    scissorRect.right =
        static_cast<LONG>(width_);

    scissorRect.bottom =
        static_cast<LONG>(height_);

    commandList_->RSSetScissorRects(
        1,
        &scissorRect
    );
}


void DirectXCommon::PostDraw() {
    UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

    // リソースバリア（RenderTarget -> Present）
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList_->ResourceBarrier(1, &barrier);

    // コマンドクローズ & 実行
    HRESULT hr = commandList_->Close();
    assert(SUCCEEDED(hr));

    ID3D12CommandList* commandLists[] = { commandList_.Get() };
    commandQueue_->ExecuteCommandLists(1, commandLists);

    // フリップ
    swapChain_->Present(1, 0);

    // フェンスでGPU完了待ち
    fenceValue_++;
    commandQueue_->Signal(fence_.Get(), fenceValue_);

    if (fence_->GetCompletedValue() < fenceValue_) {
        fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
        WaitForSingleObject(fenceEvent_, INFINITE);
    }

    // 次フレーム用にコマンドリセット
    hr = commandAllocator_->Reset();
    assert(SUCCEEDED(hr));
    hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
    assert(SUCCEEDED(hr));

    fpsLimiter_.Update();
}


void DirectXCommon::InitializeRenderTexture()
{

    const Vector4 clearColor{ 0.1f, 0.25f, 0.5f, 1.0f };

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = width_;
    resourceDesc.Height = height_;
    resourceDesc.MipLevels = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    clearValue.Color[0] = clearColor.x;
    clearValue.Color[1] = clearColor.y;
    clearValue.Color[2] = clearColor.z;
    clearValue.Color[3] = clearColor.w;

    HRESULT hr = device_->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &clearValue,
        IID_PPV_ARGS(&renderTextureResource_));
    assert(SUCCEEDED(hr));

    UINT rtvSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    renderTextureRtvHandle_ = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    renderTextureRtvHandle_.ptr += rtvSize * 2;

    device_->CreateRenderTargetView(renderTextureResource_.Get(), nullptr, renderTextureRtvHandle_);

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.NumDescriptors = 2;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = device_->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&renderTextureSrvHeap_));
    assert(SUCCEEDED(hr));

    renderTextureSrvHandleCPU_ = renderTextureSrvHeap_->GetCPUDescriptorHandleForHeapStart();
    renderTextureSrvHandleGPU_ = renderTextureSrvHeap_->GetGPUDescriptorHandleForHeapStart();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    device_->CreateShaderResourceView(renderTextureResource_.Get(), &srvDesc, renderTextureSrvHandleCPU_);

    renderTextureState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;


    UINT srvSize =
        device_->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );

    depthTextureSrvHandleCPU_ =
        renderTextureSrvHandleCPU_;

    depthTextureSrvHandleCPU_.ptr += srvSize;

    depthTextureSrvHandleGPU_ =
        renderTextureSrvHandleGPU_;

    depthTextureSrvHandleGPU_.ptr += srvSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc{};
    depthSrvDesc.Format =
        DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depthSrvDesc.Shader4ComponentMapping =
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthSrvDesc.ViewDimension =
        D3D12_SRV_DIMENSION_TEXTURE2D;
    depthSrvDesc.Texture2D.MipLevels = 1;
    depthSrvDesc.Texture2D.MostDetailedMip = 0;
    depthSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    device_->CreateShaderResourceView(
        depthStencilResource_.Get(),
        &depthSrvDesc,
        depthTextureSrvHandleCPU_
    );

}

void DirectXCommon::InitializeCopyImagePipeline()
{
    HRESULT hr = S_FALSE;

    D3D12_DESCRIPTOR_RANGE descriptorRange{};
    descriptorRange.BaseShaderRegister = 0;
    descriptorRange.NumDescriptors = 2;
    descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange.OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[2]{};
    rootParameters[0].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].DescriptorTable.pDescriptorRanges =
        &descriptorRange;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].ShaderVisibility =
        D3D12_SHADER_VISIBILITY_PIXEL;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[1].Constants.ShaderRegister = 0;
    rootParameters[1].Constants.RegisterSpace = 0;
    rootParameters[1].Constants.Num32BitValues = 12;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    
    D3D12_STATIC_SAMPLER_DESC staticSamplers[2]{};

    // s0 : Color用Linear
    staticSamplers[0].Filter =
        D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU =
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressV =
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressW =
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].ComparisonFunc =
        D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD =
        D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility =
        D3D12_SHADER_VISIBILITY_PIXEL;

    // s1 : Depth用Point
    staticSamplers[1] = staticSamplers[0];
    staticSamplers[1].Filter =
        D3D12_FILTER_MIN_MAG_MIP_POINT;
    staticSamplers[1].ShaderRegister = 1;


    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumParameters = 2;
    rootSignatureDesc.pStaticSamplers = staticSamplers;
    rootSignatureDesc.NumStaticSamplers = 2;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    hr = D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA(
                reinterpret_cast<char*>(
                    errorBlob->GetBufferPointer()
                    )
            );
        }
        assert(false);
    }

    hr = device_->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&copyImageRootSignature_)
    );
    assert(SUCCEEDED(hr));

    auto vsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/Fullscreen.VS.hlsl",
            L"vs_6_0"
        );

    auto normalPsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/CopyImage.PS.hlsl",
            L"ps_6_0"
        );

    auto grayPsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/Grayscale.PS.hlsl",
            L"ps_6_0"
        );

    auto postEffectPsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/Vignette.PS.hlsl",
            L"ps_6_0"
        );

    auto smoothingPsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/BoxFilter.PS.hlsl",
            L"ps_6_0"
        );

    auto gaussianPsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/GaussianFilter.PS.hlsl",
            L"ps_6_0"
        );

    auto outlinePsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/DepthBasedOutline.PS.hlsl",
            L"ps_6_0"
        );

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};

    psoDesc.pRootSignature =
        copyImageRootSignature_.Get();

    psoDesc.InputLayout.pInputElementDescs = nullptr;
    psoDesc.InputLayout.NumElements = 0;

    psoDesc.VS = {
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize()
    };

    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;

    psoDesc.RasterizerState.FillMode =
        D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode =
        D3D12_CULL_MODE_NONE;

    psoDesc.DepthStencilState.DepthEnable = false;
    psoDesc.DepthStencilState.StencilEnable = false;

    psoDesc.SampleMask =
        D3D12_DEFAULT_SAMPLE_MASK;

    psoDesc.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] =
        DXGI_FORMAT_R8G8B8A8_UNORM;

    psoDesc.SampleDesc.Count = 1;

    // 通常表示用 PSO
    psoDesc.PS = {
        normalPsBlob->GetBufferPointer(),
        normalPsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&normalCopyPipelineState_)
    );
    assert(SUCCEEDED(hr));

    // グレースケール用 PSO
    psoDesc.PS = {
        grayPsBlob->GetBufferPointer(),
        grayPsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&grayScalePipelineState_)
    );
    assert(SUCCEEDED(hr));

    psoDesc.PS = {
        postEffectPsBlob->GetBufferPointer(),
        postEffectPsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&postEffectPipelineState_)
    );
    assert(SUCCEEDED(hr));

    //smoothing
    psoDesc.PS = {
        smoothingPsBlob->GetBufferPointer(),
        smoothingPsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&smoothingPipelineState_)
    );
    assert(SUCCEEDED(hr));

    //GaussianFilter
    psoDesc.PS = {
    gaussianPsBlob->GetBufferPointer(),
    gaussianPsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(
            &gaussianPipelineState_
        )
    );

    assert(SUCCEEDED(hr));


    //Outline
    psoDesc.PS = {
    outlinePsBlob->GetBufferPointer(),
    outlinePsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&outlinePipelineState_)
    );

    assert(SUCCEEDED(hr));

}

void DirectXCommon::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{

    if (beforeState == afterState) {
        return;
    }

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = beforeState;

    barrier.Transition.StateAfter = afterState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList_->ResourceBarrier(1, &barrier);

}

void DirectXCommon::ResizeIfNeeded()
{

    if (!winApp_) {
        return;
    }

    uint32_t newWidth = 0;
    uint32_t newHeight = 0;

    if (
        !winApp_->ConsumeResize(
            newWidth,
            newHeight
        )
        ) {
        return;
    }

    if (
        newWidth == 0 ||
        newHeight == 0
        ) {
        return;
    }

    if (
        newWidth == width_ &&
        newHeight == height_
        ) {
        return;
    }

    WaitForGPU();

    width_ =
        newWidth;

    height_ =
        newHeight;

    // ResizeBuffers前に参照をすべて解放
    for (
        auto& resource :
        swapChainResources_
        ) {
        resource.Reset();
    }

    depthStencilResource_.Reset();
    renderTextureResource_.Reset();

    rtvDescriptorHeap_.Reset();
    dsvDescriptorHeap_.Reset();

    renderTextureSrvHeap_.Reset();

    HRESULT result =
        swapChain_->ResizeBuffers(
            static_cast<UINT>(
                GetBackBufferCount()
                ),
            width_,
            height_,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            0
        );

    assert(SUCCEEDED(result));

    InitializeRenderTargetView();
    InitializeDepthStencilView();
    InitializeRenderTexture();

}

void DirectXCommon::WaitForGPU()
{

    if (
        !commandQueue_ ||
        !fence_
        ) {
        return;
    }

    ++fenceValue_;

    HRESULT result =
        commandQueue_->Signal(
            fence_.Get(),
            fenceValue_
        );

    assert(SUCCEEDED(result));

    if (
        fence_->GetCompletedValue() <
        fenceValue_
        ) {
        result =
            fence_->SetEventOnCompletion(
                fenceValue_,
                fenceEvent_
            );

        assert(SUCCEEDED(result));

        WaitForSingleObject(
            fenceEvent_,
            INFINITE
        );
    }

}
