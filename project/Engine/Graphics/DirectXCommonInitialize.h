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

    // 蛻晄悄繧ｦ繧｣繝ｳ繝峨え逕滓・譎ゅ・WM_SIZE繧呈ｶ郁ｲｻ
    uint32_t ignoredWidth = 0;
    uint32_t ignoredHeight = 0;

    winApp_->ConsumeResize(
        ignoredWidth,
        ignoredHeight
    );

}
