void GPUParticleManager::Initialize(
    DirectXCommon* dxCommon,
    SrvManager* srvManager,
    TextureManager* textureManager
) {
    assert(dxCommon);
    assert(srvManager);
    assert(textureManager);

    dxCommon_ = dxCommon;
    srvManager_ = srvManager;
    textureManager_ = textureManager;

    textureHandle_ =
        textureManager_->LoadTexture("Resources/white.png");

    CreateBuffers();
    CreateDescriptors();
    CreateGraphicsRootSignature();
    CreateGraphicsPipelineState();
    CreateComputeRootSignature();
    CreateComputePipelineState();
    CreateMesh();
    InitializeParticlesOnGPU();
}

