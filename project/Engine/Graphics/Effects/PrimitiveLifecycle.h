void Primitive::Emit(const Vector3& position)
{

    int count = settings_.count;

    if (count < 1) {
        count = 1;
    }

    if (count > static_cast<int>(kMaxParticles)) {
        count = static_cast<int>(kMaxParticles);
    }

    Emit(
        position,
        static_cast<uint32_t>(count)
    );

}

void Primitive::Initialize(
    DirectXCommon* dxCommon,
    TextureManager* textureManager
) {
    assert(dxCommon);
    assert(textureManager);

    dxCommon_ = dxCommon;
    textureManager_ = textureManager;

    textureHandle_ =
        textureManager_->LoadTexture(
            "Resources/white.png"
        );

    CreateRootSignature();
    CreatePipelineState();
    CreateMesh();

    const UINT bufferSize =
        sizeof(InstanceData) * kMaxParticles;

    instancingBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            dxCommon_->GetDevice(),
            bufferSize
        );

    instancingDataMapped_ =
        D3DResourceHelper::Map<InstanceData>(
            instancingBuffer_.Get()
        );

    instancingBufferView_.BufferLocation =
        instancingBuffer_
        ->GetGPUVirtualAddress();

    instancingBufferView_.SizeInBytes =
        bufferSize;

    instancingBufferView_.StrideInBytes =
        sizeof(InstanceData);
}


