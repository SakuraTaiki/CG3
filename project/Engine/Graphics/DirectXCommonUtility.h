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


    for (
        auto& resource :
        swapChainResources_
        ) {
        resource.Reset();
    }

    depthStencilResource_.Reset();
    renderTextureResource_.Reset();
    postEffectTextureResource_.Reset();

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

    if (hasDissolveMaskSource_) {
        SetDissolveMaskSrv(dissolveMaskSourceHandle_);
    }

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

