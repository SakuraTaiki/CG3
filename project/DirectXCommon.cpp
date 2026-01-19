#include "DirectXCommon.h"
#include <cassert>
#include "StringUtility.h"
#include "Logger.h"
#include <format>
#include <dxcapi.h>

// ImGui
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/DirectXTex/d3dx12.h"
#include <thread>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

using namespace Microsoft::WRL;

const uint32_t DirectXCommon::kMaxSRVCount = 512;


void DirectXCommon::Initialize(WinApp* winApp) {

    assert(winApp);
    //WinApp
    this->winApp = winApp;

    

    ////Command
    //this->commandAllocator = commandAllocator;
    //this->commandList = commandList;

    ////SwapChain
    //this->swapChain = swapChain;
    //this->swapChainResources[0] = swapChainResources[0];
    //this->swapChainResources[1] = swapChainResources[1];

    //this->rtvDescriptorHeap = rtvDescriptorHeap;
    //this->srvDescriptorHeap = srvDescriptorHeap;
    //this->dsvDescriptorHeap = dsvDescriptorHeap;

    //this->rtvHandles[0] = rtvHandles[0];
    //this->rtvHandles[1] = rtvHandles[1];

    //this->depthStencilResource = depthStencilResource;

    //this->fence = fence;
    //this->fenceEvent = fenceEvent;
    //this->fenceValue = fenceValue;

    //this->viewport = viewport;
    //this->scissorRect = scissorRect;

#ifdef _DEBUG
    ComPtr<ID3D12Debug1> debugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        debugController->SetEnableGPUBasedValidation(true);
    }
#endif

    //=== DXGIファクトリ ===//
    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
    assert(SUCCEEDED(hr));

    //=== アダプタ選択 ===//
    ComPtr<IDXGIAdapter4> useAdapter = nullptr;

    for (int i = 0;
        dxgiFactory->EnumAdapterByGpuPreference(i,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND;
        ++i) {

        DXGI_ADAPTER_DESC3 adapterDesc{};
        hr = useAdapter->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr));

        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
            Logger::Log(Logger::logStream,
                StringUtility::ConvertString(
                    std::format(L"use Adapter: {}\n", adapterDesc.Description)
                ));
            break;
        }
        useAdapter = nullptr;
    }
    assert(useAdapter != nullptr);

    //=== デバイス生成 ===//
    D3D_FEATURE_LEVEL featureLevels[] =
    { D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0 };
    const char* featureLevelStrings[] = { "12.2","12.1","12.0" };

    for (size_t i = 0; i < _countof(featureLevels); ++i) {
        hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));
        if (SUCCEEDED(hr)) {
            Logger::Log(Logger::logStream,
                std::string("FeatureLevel: ") + featureLevelStrings[i]);
            break;
        }
    }
    assert(device != nullptr);

    fenceValue = 0;

     hr = device->CreateFence(
        fenceValue,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(fence.GetAddressOf())
    );
    assert(SUCCEEDED(hr));

    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(fenceEvent != nullptr);

    //=== INFO QUEUE ===//
#ifdef _DEBUG
    ComPtr<ID3D12InfoQueue> infoqueue = nullptr;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoqueue)))) {
        infoqueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        infoqueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        infoqueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
    }
#endif

    //=== コマンドキュー ===//
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
    assert(SUCCEEDED(hr));

    //=== コマンドアロケータ ===//
    
    hr = device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator));
    assert(SUCCEEDED(hr));

    //=== コマンドリスト ===//
    hr = device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator.Get(), nullptr,
        IID_PPV_ARGS(&commandList));
    assert(SUCCEEDED(hr));

    //=== スワップチェイン ===//
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = WinApp::kClientWidth;
    swapChainDesc.Height = WinApp::kClientHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    hr = dxgiFactory->CreateSwapChainForHwnd(
        commandQueue.Get(), winApp->GetHwnd(),
        &swapChainDesc,
        nullptr, nullptr,
        reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
    assert(SUCCEEDED(hr));

    //=== RTV/SRV/DSV Heap ===//
    rtvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
    srvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,kMaxSRVCount, true);
    dsvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

    descriptorSizeSRV =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    //=== バックバッファ取得 ===//
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
    hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));

    //=== RTV作成 ===//
    D3D12_CPU_DESCRIPTOR_HANDLE rtvStart = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    rtvHandles[0] = rtvStart;
    device->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);

    rtvHandles[1].ptr = rtvHandles[0].ptr +
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    device->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);


    //=== DepthStencil ===//
    depthStencilResource =
        CreateDepthStencilTextureResource(device, WinApp::kClientWidth, WinApp::kClientHeight);

    device->CreateDepthStencilView(
        depthStencilResource.Get(), nullptr,
        dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


    //=== Viewport & Scissor ===//
    viewport.Width = WinApp::kClientWidth;
    viewport.Height = WinApp::kClientHeight;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MinDepth = 0.f;
    viewport.MaxDepth = 1.f;

    scissorRect.left = 0;
    scissorRect.right = WinApp::kClientWidth;
    scissorRect.top = 0;
    scissorRect.bottom = WinApp::kClientHeight;


    //=== ImGui ===//
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(winApp->GetHwnd());
    ImGui_ImplDX12_Init(
        device.Get(), 2,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        srvDescriptorHeap.Get(),
        srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    //FPS固定初期化
    InitializeFixFPS();
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    handle.ptr += static_cast<SIZE_T>(descriptorSizeSRV) * index;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index)
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle =
        srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

    handle.ptr += static_cast<UINT64>(descriptorSizeSRV) * index;
    return handle;
}


//==============================
// PreDraw
//==============================
void DirectXCommon::PreDraw() {
    UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

    // Present → RenderTarget
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
        dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);

    float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
    commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap.Get() };
    commandList->SetDescriptorHeaps(1, heaps);

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
}


//==============================
// PostDraw（まだ中身は空）
//==============================
void DirectXCommon::PostDraw() {

    UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

    //==============================
    // RenderTarget → Present
    //==============================
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

    commandList->ResourceBarrier(1, &barrier);

    //==============================
    // Close & Execute
    //==============================
    HRESULT hr = commandList->Close();
    assert(SUCCEEDED(hr));

    ID3D12CommandList* lists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(1, lists);

    swapChain->Present(1, 0);

    UpdateFixFPS();

    //==============================
    // Fence 同期
    //==============================
    commandQueue->Signal(fence.Get(), ++fenceValue);

    if (fence->GetCompletedValue() < fenceValue) {
        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    //==============================
    // 次フレーム準備
    //==============================
    hr = commandAllocator->Reset();
    assert(SUCCEEDED(hr));

    hr = commandList->Reset(commandAllocator.Get(), nullptr);
    assert(SUCCEEDED(hr));
}

Microsoft::WRL::ComPtr<IDxcBlob>
DirectXCommon::CompileShander(const std::wstring& filePath, const wchar_t* profile)
{
    std::ofstream os("log.txt", std::ios::app);

    Logger::Log(os,
        StringUtility::ConvertString(
            std::format(L"Begin CompileShader,path:{}, profile:{}\n", filePath, profile)));

    HRESULT hr;

    //==============================
    // DXC Utils
    //==============================
    ComPtr<IDxcUtils> dxcUtils;
    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(dxcUtils.GetAddressOf()));
    assert(SUCCEEDED(hr));

    //==============================
    // DXC Compiler
    //==============================
    ComPtr<IDxcCompiler3> dxCompiler;
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(dxCompiler.GetAddressOf()));
    assert(SUCCEEDED(hr));

    //==============================
    // Include Handler
    //==============================
    ComPtr<IDxcIncludeHandler> includeHandler;
    hr = dxcUtils->CreateDefaultIncludeHandler(includeHandler.GetAddressOf());
    assert(SUCCEEDED(hr));

    //==============================
    // Load HLSL
    //==============================
    ComPtr<IDxcBlobEncoding> shaderSource;
    hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, shaderSource.GetAddressOf());
    assert(SUCCEEDED(hr));

    DxcBuffer sourceBuffer{};
    sourceBuffer.Ptr = shaderSource->GetBufferPointer();
    sourceBuffer.Size = shaderSource->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_UTF8;

    //==============================
    // Compile arguments
    //==============================

    LPCWSTR arguments[] = {
        filePath.c_str(),
        L"-E", L"main",
        L"-T", profile,
        L"-Zi", L"-Qembed_debug",
        L"-Od",
        L"-Zpr"
    };

    //==============================
    // Compile
    //==============================
    ComPtr<IDxcResult> shaderResult;
    hr = dxCompiler->Compile(
        &sourceBuffer,
        arguments,
        _countof(arguments),
        includeHandler.Get(),
        IID_PPV_ARGS(shaderResult.GetAddressOf()));
    assert(SUCCEEDED(hr));

    //==============================
    // Error check
    //==============================
    ComPtr<IDxcBlobUtf8> errors;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr);
    if (errors && errors->GetStringLength() != 0) {
        Logger::Log(os, errors->GetStringPointer());
        assert(false);
    }

    //==============================
    // Get compiled shader
    //==============================
    ComPtr<IDxcBlob> shaderBlob;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shaderBlob.GetAddressOf()), nullptr);
    assert(SUCCEEDED(hr));

    Logger::Log(os,
        StringUtility::ConvertString(
            std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));

    return shaderBlob;
}
Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateBufferResource(size_t sizeInBytes)
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

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateTextureResource(const DirectX::TexMetadata& metadata)
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

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages)
{
    std::vector<D3D12_SUBRESOURCE_DATA>subresources;
	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
	ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(intermediateSize);
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

DirectX::ScratchImage DirectXCommon::LoadTexture(const std::string& filePath)
{
    DirectX::ScratchImage image{};
    std::wstring filePathW = StringUtility::ConvertString(filePath);
    HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
    assert(SUCCEEDED(hr));

    //ミニマップの作成
    DirectX::ScratchImage mipImages{};
    hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
    assert(SUCCEEDED(hr));

    //ミップマップのデータを返す
    return mipImages;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = heapType;
    desc.NumDescriptors = numDescriptors;
    desc.Flags = shaderVisible ?
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE :
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
    HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
    assert(SUCCEEDED(hr));

    return heap;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(descriptorSize) * index;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(descriptorSize) * index;
    return handle;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height)
{
    assert(device);

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clearValue.DepthStencil.Depth = 1.0f;

    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&resource));

    assert(SUCCEEDED(hr));

    return resource;
}

void DirectXCommon::InitializeFixFPS()
{
    reference_ = std::chrono::steady_clock::now();
}

void DirectXCommon::UpdateFixFPS()
{
    //60/1秒ピッタリな時間
    const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));

    const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

    if (elapsed < kMinCheckTime) {
        //1/60秒経過するまで微小なスリープを繰り返す
        while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
            //1秒マイクロスリープ
            std::this_thread::sleep_for(std::chrono::microseconds(1));

        }
    }
    reference_ = std::chrono::steady_clock::now();
}