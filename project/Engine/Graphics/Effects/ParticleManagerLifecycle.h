void ParticleManager::Initialize(DirectXCommon* dxCommon, TextureManager* textureManager) {
    assert(dxCommon);
    dxCommon_ = dxCommon;
    textureManager_ = textureManager;

    
    textureHandle_ = textureManager_->LoadTexture("Resources/white.png");

    // 2. 繝代う繝励Λ繧､繝ｳ逕滓・
    CreateRootSignature();
    CreatePipelineState();

    
    CreateMesh();

    
    {
        auto device = dxCommon_->GetDevice();
        UINT size = sizeof(InstanceData) * kMaxParticles;

        instancingBuffer_ =
            D3DResourceHelper::CreateUploadBuffer(
                device,
                size
            );

        instancingDataMapped_ =
            D3DResourceHelper::Map<InstanceData>(
                instancingBuffer_.Get()
            );

        instancingBufferView_.BufferLocation = instancingBuffer_->GetGPUVirtualAddress();
        instancingBufferView_.SizeInBytes = size;
        instancingBufferView_.StrideInBytes = sizeof(InstanceData);
    }
}


