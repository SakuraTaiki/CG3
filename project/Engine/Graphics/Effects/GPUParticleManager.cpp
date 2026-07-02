#include "GPUParticleManager.h"

#include <algorithm>
#include <cassert>
#include <cstring>

#include "D3DResourceHelper.h"

using Microsoft::WRL::ComPtr;


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

void GPUParticleManager::Update(
    const Matrix4x4& viewMatrix,
    const Matrix4x4& projectionMatrix
) {
    constexpr float deltaTime = 1.0f / 60.0f;
    totalTime_ += deltaTime;

    viewProjectionData_->billboard =
        Math::MakeBillboardMatrix(viewMatrix);
    viewProjectionData_->viewProjection =
        Math::Multiply(viewMatrix, projectionMatrix);

    updateData_->deltaTime = deltaTime;
    updateData_->totalTime = totalTime_;
    updateData_->particleCount = kMaxParticles;

    perFrameData_->time = totalTime_;
    perFrameData_->deltaTime = deltaTime;

    TransitionParticleResource(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
    srvManager_->PreDraw();

   

    if (emitRequested_)
    {
        DispatchEmit();

        // Emit CSの書き込み完了後に
        // Update CSを実行させる。
        D3D12_RESOURCE_BARRIER barriers[2]{};

        barriers[0].Type =
            D3D12_RESOURCE_BARRIER_TYPE_UAV;

        barriers[0].Flags =
            D3D12_RESOURCE_BARRIER_FLAG_NONE;

        barriers[0].UAV.pResource =
            particleBuffer_.Get();

        barriers[1].Type =
            D3D12_RESOURCE_BARRIER_TYPE_UAV;

        barriers[1].Flags =
            D3D12_RESOURCE_BARRIER_FLAG_NONE;

        barriers[1].UAV.pResource =
            freeCounterBuffer_.Get();

        commandList->ResourceBarrier(
            _countof(barriers),
            barriers
        );

        emitRequested_ = false;
    }

    commandList->SetComputeRootSignature(
        computeRootSignature_.Get()
    );

    commandList->SetPipelineState(
        computePipelineState_.Get()
    );

    // u0 : gParticles
    commandList->SetComputeRootDescriptorTable(
        0,
        srvManager_->GetGPUDescriptorHandle(
            particleUavIndex_
        )
    );

    // b2 : UpdateData
    commandList->SetComputeRootConstantBufferView(
        4,
        updateBuffer_->GetGPUVirtualAddress()
    );

    commandList->Dispatch(
        (kMaxParticles + kThreadCount - 1) /
        kThreadCount,
        1,
        1
    );

    // Update CSの書き込みを、
    // 後続の描画や次の処理から参照可能にする。
    D3D12_RESOURCE_BARRIER updateBarrier{};

    updateBarrier.Type =
        D3D12_RESOURCE_BARRIER_TYPE_UAV;

    updateBarrier.Flags =
        D3D12_RESOURCE_BARRIER_FLAG_NONE;

    updateBarrier.UAV.pResource =
        particleBuffer_.Get();

    commandList->ResourceBarrier(
        1,
        &updateBarrier
    );
}

void GPUParticleManager::Draw()
{

    if (!settings_.enabled)
    {
        return;
    }

    ID3D12GraphicsCommandList* commandList =
        dxCommon_->GetCommandList();

    TransitionParticleResource(
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );

    commandList->SetPipelineState(
        graphicsPipelineState_.Get()
    );
    commandList->SetGraphicsRootSignature(
        graphicsRootSignature_.Get()
    );
    commandList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );
    commandList->IASetVertexBuffers(
        0,
        1,
        &vertexBufferView_
    );

    commandList->SetGraphicsRootDescriptorTable(
        0,
        textureManager_->GetSrvHandleGPU(textureHandle_)
    );
    commandList->SetGraphicsRootDescriptorTable(
        1,
        srvManager_->GetGPUDescriptorHandle(particleSrvIndex_)
    );
    commandList->SetGraphicsRootConstantBufferView(
        2,
        viewProjectionBuffer_->GetGPUVirtualAddress()
    );

    commandList->DrawInstanced(
        6,
        kMaxParticles,
        0,
        0
    );
}

void GPUParticleManager::Emit(
    const Vector3& position,
    uint32_t count,
    float sizeMultiplier
) {
    if (!settings_.enabled)
    {
        return;
    }

    sizeMultiplier =
        (std::max)(sizeMultiplier, 0.01f);

    const int emitCount =
        std::clamp(
            settings_.fireCount,
            1,
            static_cast<int>(kMaxParticles)
        );

    emitterData_->translate = position;

    emitterData_->radius =
        settings_.spawnRadius *
        sizeMultiplier;

    emitterData_->count =
        static_cast<uint32_t>(emitCount);

    emitterData_->frequency = 0.5f;
    emitterData_->frequencyTime = 0.0f;
    emitterData_->emit = 1;
    emitterData_->effectType = 0.0f;

    emitterData_->sizeMultiplier =
        sizeMultiplier *
        (std::max)(settings_.particleScale, 0.01f);

    emitterData_->mainColor =
        settings_.fireMainColor;

    emitterData_->subColor =
        settings_.fireSubColor;

    emitRequested_ = true;

    // 既存APIとの互換性のため引数は残す。
    (void)count;
}

void GPUParticleManager::EmitSakura(
    const Vector3& position,
    uint32_t count,
    float sizeMultiplier
) {
    if (!settings_.enabled)
    {
        return;
    }

    sizeMultiplier =
        (std::max)(sizeMultiplier, 0.01f);

    const int emitCount =
        std::clamp(
            settings_.sakuraCount,
            1,
            static_cast<int>(kMaxParticles)
        );

    emitterData_->translate = position;

    emitterData_->radius =
        settings_.spawnRadius *
        sizeMultiplier;

    emitterData_->count =
        static_cast<uint32_t>(emitCount);

    emitterData_->frequency = 0.5f;
    emitterData_->frequencyTime = 0.0f;
    emitterData_->emit = 1;
    emitterData_->effectType = 1.0f;

    emitterData_->sizeMultiplier =
        sizeMultiplier *
        (std::max)(settings_.particleScale, 0.01f);

    emitterData_->mainColor =
        settings_.sakuraMainColor;

    emitterData_->subColor =
        settings_.sakuraSubColor;

    emitRequested_ = true;

    (void)count;
}

void GPUParticleManager::CreateBuffers()
{
    ID3D12Device* device = dxCommon_->GetDevice();
    const UINT particleBufferSize =
        sizeof(ParticleData) * kMaxParticles;

    D3D12_HEAP_PROPERTIES defaultHeap{};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC particleDesc =
        D3DResourceHelper::MakeBufferResourceDesc(particleBufferSize);
    particleDesc.Flags =
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    HRESULT hr = device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &particleDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&particleBuffer_)
    );
    assert(SUCCEEDED(hr));

    // CounterはGPUからのみ読み書きする。
    D3D12_RESOURCE_DESC counterDesc =
        D3DResourceHelper::MakeBufferResourceDesc(sizeof(int32_t));
    counterDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    HRESULT counterHr = device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &counterDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&freeCounterBuffer_));
    assert(SUCCEEDED(counterHr));

    emitterBuffer_ = D3DResourceHelper::CreateUploadBuffer(
        device,
        D3DResourceHelper::AlignConstantBufferSize(sizeof(EmitterSphere)));
    emitterData_ = D3DResourceHelper::Map<EmitterSphere>(emitterBuffer_.Get());
    *emitterData_ = {};
    emitterData_->count = 10;
    emitterData_->frequency = 0.5f;
    emitterData_->radius = 1.0f;
    emitterData_->sizeMultiplier = 1.0f;

    emitterData_->mainColor =
        settings_.fireMainColor;

    emitterData_->subColor =
        settings_.fireSubColor;

    perFrameBuffer_ = D3DResourceHelper::CreateUploadBuffer(
        device,
        D3DResourceHelper::AlignConstantBufferSize(sizeof(PerFrame)));
    perFrameData_ = D3DResourceHelper::Map<PerFrame>(perFrameBuffer_.Get());
    *perFrameData_ = {};

    viewProjectionBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            device,
            D3DResourceHelper::AlignConstantBufferSize(
                sizeof(ViewProjectionData)
            )
        );
    viewProjectionData_ =
        D3DResourceHelper::Map<ViewProjectionData>(
            viewProjectionBuffer_.Get()
        );

    updateBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            device,
            D3DResourceHelper::AlignConstantBufferSize(
                sizeof(UpdateData)
            )
        );
    updateData_ =
        D3DResourceHelper::Map<UpdateData>(
            updateBuffer_.Get()
        );
}

void GPUParticleManager::CreateDescriptors()
{
    particleSrvIndex_ = srvManager_->Allocate();
    srvManager_->CreateSRVForStructuredBuffer(
        particleSrvIndex_,
        particleBuffer_.Get(),
        kMaxParticles,
        sizeof(ParticleData)
    );

    particleUavIndex_ = srvManager_->Allocate();
    srvManager_->CreateUAVForStructuredBuffer(
        particleUavIndex_,
        particleBuffer_.Get(),
        kMaxParticles,
        sizeof(ParticleData)
    );

    freeCounterUavIndex_ = srvManager_->Allocate();
    srvManager_->CreateUAVForStructuredBuffer(
        freeCounterUavIndex_,
        freeCounterBuffer_.Get(),
        1,
        sizeof(int32_t));
}

void GPUParticleManager::CreateGraphicsRootSignature()
{
    D3D12_DESCRIPTOR_RANGE ranges[2]{};
    ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[0].NumDescriptors = 1;
    ranges[0].BaseShaderRegister = 0;
    ranges[0].OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[1].NumDescriptors = 1;
    ranges[1].BaseShaderRegister = 1;
    ranges[1].OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[3]{};
    rootParameters[0].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = &ranges[0];
    rootParameters[0].ShaderVisibility =
        D3D12_SHADER_VISIBILITY_PIXEL;

    rootParameters[1].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = &ranges[1];
    rootParameters[1].ShaderVisibility =
        D3D12_SHADER_VISIBILITY_VERTEX;

    rootParameters[2].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[2].Descriptor.ShaderRegister = 0;
    rootParameters[2].ShaderVisibility =
        D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility =
        D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = _countof(rootParameters);
    desc.pParameters = rootParameters;
    desc.NumStaticSamplers = 1;
    desc.pStaticSamplers = &sampler;
    desc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(
        &desc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &blob,
        &errorBlob
    );
    assert(SUCCEEDED(hr));

    hr = dxCommon_->GetDevice()->CreateRootSignature(
        0,
        blob->GetBufferPointer(),
        blob->GetBufferSize(),
        IID_PPV_ARGS(&graphicsRootSignature_)
    );
    assert(SUCCEEDED(hr));
}

void GPUParticleManager::CreateGraphicsPipelineState()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    auto vsBlob = dxCommon_->CompileShader(
        L"Resources/shaders/hlsl/GPUParticle.VS.hlsl",
        L"vs_6_0"
    );
    auto psBlob = dxCommon_->CompileShader(
        L"Resources/shaders/hlsl/GPUParticle.PS.hlsl",
        L"ps_6_0"
    );

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = graphicsRootSignature_.Get();
    desc.InputLayout = { inputLayout, _countof(inputLayout) };
    desc.VS = {
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize()
    };
    desc.PS = {
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize()
    };

    D3DResourceHelper::SetParticlePipelineDefaults(desc);

    HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(
        &desc,
        IID_PPV_ARGS(&graphicsPipelineState_)
    );
    assert(SUCCEEDED(hr));
}


void GPUParticleManager::CreateComputeRootSignature()
{
    D3D12_DESCRIPTOR_RANGE ranges[2]{};

    ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    ranges[0].NumDescriptors = 1;
    ranges[0].BaseShaderRegister = 0;
    ranges[0].OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    ranges[1].NumDescriptors = 1;
    ranges[1].BaseShaderRegister = 1;
    ranges[1].OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // 0:u0 Particle, 1:u1 Counter, 2:b0 Emitter,
    // 3:b1 PerFrame, 4:b2 UpdateData
    D3D12_ROOT_PARAMETER rootParameters[5]{};

    for (UINT i = 0; i < 2; ++i)
    {
        rootParameters[i].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[i].DescriptorTable.pDescriptorRanges = &ranges[i];
        rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }

    for (UINT i = 0; i < 3; ++i)
    {
        rootParameters[i + 2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[i + 2].Descriptor.ShaderRegister = i;
        rootParameters[i + 2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }

    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = _countof(rootParameters);
    desc.pParameters = rootParameters;

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(
        &desc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &blob,
        &errorBlob);
    assert(SUCCEEDED(hr));

    hr = dxCommon_->GetDevice()->CreateRootSignature(
        0,
        blob->GetBufferPointer(),
        blob->GetBufferSize(),
        IID_PPV_ARGS(&computeRootSignature_));
    assert(SUCCEEDED(hr));
}


void GPUParticleManager::CreateComputePipelineState()
{
    auto updateCSBlob = dxCommon_->CompileShader(
        L"Resources/shaders/hlsl/GPUParticle.CS.hlsl",
        L"cs_6_0"
    );

    auto initializeCSBlob = dxCommon_->CompileShader(
        L"Resources/shaders/hlsl/InitializeGPUParticle.CS.hlsl",
        L"cs_6_0"
    );

    auto emitCSBlob = dxCommon_->CompileShader(
        L"Resources/shaders/hlsl/EmitParticle.CS.hlsl",
        L"cs_6_0"
    );

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = computeRootSignature_.Get();

    // 更新用
    desc.CS = {
        updateCSBlob->GetBufferPointer(),
        updateCSBlob->GetBufferSize()
    };

    HRESULT hr =
        dxCommon_->GetDevice()->CreateComputePipelineState(
            &desc,
            IID_PPV_ARGS(&computePipelineState_)
        );
    assert(SUCCEEDED(hr));

    // 初期化用
    desc.CS = {
        initializeCSBlob->GetBufferPointer(),
        initializeCSBlob->GetBufferSize()
    };

    hr = dxCommon_->GetDevice()->CreateComputePipelineState(
        &desc,
        IID_PPV_ARGS(&initializePipelineState_)
    );
    assert(SUCCEEDED(hr));

    // 発生用
    desc.CS = {
        emitCSBlob->GetBufferPointer(),
        emitCSBlob->GetBufferSize()
    };

    hr = dxCommon_->GetDevice()->CreateComputePipelineState(
        &desc,
        IID_PPV_ARGS(&emitPipelineState_)
    );
    assert(SUCCEEDED(hr));
}


void GPUParticleManager::CreateMesh()
{

    VertexData vertices[] = {
     {{-0.5f,  0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
     {{ 0.5f,  0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
     {{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},

     {{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
     {{ 0.5f,  0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
     {{ 0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
    };

    const UINT size = sizeof(vertices);

    vertexBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            dxCommon_->GetDevice(),
            size
        );

    VertexData* data =
        D3DResourceHelper::Map<VertexData>(
            vertexBuffer_.Get()
        );

    std::memcpy(data, vertices, size);
    vertexBuffer_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation =
        vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = size;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

}

void GPUParticleManager::InitializeParticlesOnGPU()
{

    updateData_->deltaTime = 0.0f;
    updateData_->totalTime = 0.0f;
    updateData_->particleCount = kMaxParticles;

    TransitionParticleResource(
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );

    ID3D12GraphicsCommandList* commandList =
        dxCommon_->GetCommandList();

    srvManager_->PreDraw();

    commandList->SetComputeRootSignature(
        computeRootSignature_.Get()
    );
    commandList->SetPipelineState(
        initializePipelineState_.Get()
    );
    commandList->SetComputeRootDescriptorTable(
        0,
        srvManager_->GetGPUDescriptorHandle(particleUavIndex_)
    );

    commandList->SetComputeRootDescriptorTable(
        1,
        srvManager_->GetGPUDescriptorHandle(freeCounterUavIndex_)
    );


    // b2 : UpdateData
    commandList->SetComputeRootConstantBufferView(
        4,
        updateBuffer_->GetGPUVirtualAddress()
    );

    commandList->Dispatch(
        (kMaxParticles + kThreadCount - 1) / kThreadCount,
        1,
        1
    );


    D3D12_RESOURCE_BARRIER barriers[2]{};
    barriers[0].Type =
        D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[0].UAV.pResource =
        particleBuffer_.Get();

    barriers[1].Type =
        D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[1].UAV.pResource =
        freeCounterBuffer_.Get();

    commandList->ResourceBarrier(
        _countof(barriers),
        barriers
    );

}


void GPUParticleManager::DispatchEmit()
{
    ID3D12GraphicsCommandList* commandList =
        dxCommon_->GetCommandList();

    commandList->SetComputeRootSignature(
        computeRootSignature_.Get()
    );

    commandList->SetPipelineState(
        emitPipelineState_.Get()
    );

    // u0 : Particle
    commandList->SetComputeRootDescriptorTable(
        0,
        srvManager_->GetGPUDescriptorHandle(
            particleUavIndex_
        )
    );

    // u1 : Counter
    commandList->SetComputeRootDescriptorTable(
        1,
        srvManager_->GetGPUDescriptorHandle(
            freeCounterUavIndex_
        )
    );

    // b0 : EmitterSphere
    commandList->SetComputeRootConstantBufferView(
        2,
        emitterBuffer_->GetGPUVirtualAddress()
    );

    // b1 : PerFrame
    commandList->SetComputeRootConstantBufferView(
        3,
        perFrameBuffer_->GetGPUVirtualAddress()
    );

    commandList->Dispatch(1, 1, 1);
}


void GPUParticleManager::TransitionParticleResource(
    D3D12_RESOURCE_STATES afterState
) {
    if (particleResourceState_ == afterState) {
        return;
    }

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = particleBuffer_.Get();
    barrier.Transition.StateBefore = particleResourceState_;
    barrier.Transition.StateAfter = afterState;
    barrier.Transition.Subresource =
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    dxCommon_->GetCommandList()->ResourceBarrier(
        1,
        &barrier
    );

    particleResourceState_ = afterState;
}
