#include "GPUParticleManager.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <numbers>
#include <random>

#include "D3DResourceHelper.h"

using Microsoft::WRL::ComPtr;

namespace {
    std::random_device seed;
    std::mt19937_64 randomEngine(seed());

    float RandomFloat(float minValue, float maxValue)
    {
        std::uniform_real_distribution<float> distribution(minValue, maxValue);
        return distribution(randomEngine);
    }
}

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
}

void GPUParticleManager::Update(
    const Matrix4x4& viewMatrix,
    const Matrix4x4& projectionMatrix
) {
    constexpr float deltaTime = 1.0f / 60.0f;
    totalTime_ += deltaTime;

    UploadPendingParticles();

    viewProjectionData_->billboard =
        Math::MakeBillboardMatrix(viewMatrix);
    viewProjectionData_->viewProjection =
        Math::Multiply(viewMatrix, projectionMatrix);

    updateData_->deltaTime = deltaTime;
    updateData_->totalTime = totalTime_;
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
        computePipelineState_.Get()
    );
    commandList->SetComputeRootDescriptorTable(
        0,
        srvManager_->GetGPUDescriptorHandle(particleUavIndex_)
    );
    commandList->SetComputeRootConstantBufferView(
        1,
        updateBuffer_->GetGPUVirtualAddress()
    );
    commandList->Dispatch(
        (kMaxParticles + kThreadCount - 1) / kThreadCount,
        1,
        1
    );

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = particleBuffer_.Get();
    commandList->ResourceBarrier(1, &barrier);
}

void GPUParticleManager::Draw()
{
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
    sizeMultiplier = (std::max)(sizeMultiplier, 0.01f);

    for (uint32_t index = 0; index < count; ++index) {
        const uint32_t slot = emitIndex_;
        emitIndex_ = (emitIndex_ + 1) % kMaxParticles;

        const float angle =
            RandomFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
        const float speed =
            RandomFloat(0.025f, 0.085f) * sizeMultiplier;
        const float width =
            RandomFloat(0.16f, 0.42f) * sizeMultiplier;
        const float height =
            RandomFloat(0.38f, 0.95f) * sizeMultiplier;

        ParticleData particle{};
        particle.translate = {
            position.x + RandomFloat(-0.3f, 0.3f) * sizeMultiplier,
            position.y + RandomFloat(-0.2f, 0.2f) * sizeMultiplier,
            position.z
        };
        particle.scale = { width, height, 1.0f };
        particle.startScale = particle.scale;
        particle.velocity = {
            std::cos(angle) * speed * 0.3f,
            RandomFloat(0.015f, 0.055f) * sizeMultiplier,
            std::sin(angle) * speed * 0.1f
        };
        particle.acceleration = { 0.0f, -0.0007f, 0.0f };
        particle.color = {
            1.0f,
            RandomFloat(0.12f, 0.55f),
            RandomFloat(0.01f, 0.08f),
            1.0f
        };
        particle.rotateZ = RandomFloat(-0.4f, 0.4f);
        particle.angularVelocity = RandomFloat(-0.03f, 0.03f);
        particle.lifeTime = 0.0f;
        particle.maxTime = RandomFloat(0.32f, 0.72f);
        particle.effectType = 0.0f;
        particle.isAlive = 1.0f;

        uploadData_[slot] = particle;
        pendingIndices_.push_back(slot);
    }
}

void GPUParticleManager::EmitSakura(
    const Vector3& position,
    uint32_t count,
    float sizeMultiplier
) {
    sizeMultiplier = (std::max)(sizeMultiplier, 0.01f);

    for (uint32_t index = 0; index < count; ++index) {
        const uint32_t slot = emitIndex_;
        emitIndex_ = (emitIndex_ + 1) % kMaxParticles;

        const float angle =
            RandomFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
        const float radius =
            std::sqrt(RandomFloat(0.0f, 1.0f)) * 0.6f * sizeMultiplier;
        const float size =
            RandomFloat(0.08f, 0.22f) * sizeMultiplier;

        ParticleData particle{};
        particle.translate = {
            position.x + std::cos(angle) * radius,
            position.y + RandomFloat(-0.25f, 0.35f) * sizeMultiplier,
            position.z + std::sin(angle) * radius * 0.35f
        };
        particle.scale = {
            size,
            size * RandomFloat(1.2f, 1.7f),
            1.0f
        };
        particle.startScale = particle.scale;
        particle.velocity = {
            std::cos(angle) * RandomFloat(0.025f, 0.07f) * sizeMultiplier,
            RandomFloat(0.006f, 0.03f) * sizeMultiplier,
            std::sin(angle) * RandomFloat(0.01f, 0.035f) * sizeMultiplier
        };
        particle.acceleration = { 0.0f, -0.0012f, 0.0f };
        particle.color = {
            1.0f,
            RandomFloat(0.35f, 0.78f),
            RandomFloat(0.62f, 0.92f),
            1.0f
        };
        particle.rotateZ =
            RandomFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
        particle.angularVelocity = RandomFloat(-0.12f, 0.12f);
        particle.lifeTime = 0.0f;
        particle.maxTime = RandomFloat(0.7f, 1.35f);
        particle.effectType = 1.0f;
        particle.isAlive = 1.0f;

        uploadData_[slot] = particle;
        pendingIndices_.push_back(slot);
    }
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

    uploadBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            device,
            particleBufferSize
        );
    uploadData_ =
        D3DResourceHelper::Map<ParticleData>(
            uploadBuffer_.Get()
        );
    
    pendingIndices_.reserve(kMaxParticles);

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
    D3D12_DESCRIPTOR_RANGE range{};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = 0;
    range.OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[2]{};
    rootParameters[0].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = &range;
    rootParameters[0].ShaderVisibility =
        D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[1].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor.ShaderRegister = 0;
    rootParameters[1].ShaderVisibility =
        D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = _countof(rootParameters);
    desc.pParameters = rootParameters;

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
        IID_PPV_ARGS(&computeRootSignature_)
    );
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

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = computeRootSignature_.Get();
    desc.CS = {
        updateCSBlob->GetBufferPointer(),
        updateCSBlob->GetBufferSize()
    };

    HRESULT hr = dxCommon_->GetDevice()->CreateComputePipelineState(
        &desc,
        IID_PPV_ARGS(&computePipelineState_)
    );
    assert(SUCCEEDED(hr));

    desc.CS = {
        initializeCSBlob->GetBufferPointer(),
        initializeCSBlob->GetBufferSize()
    };

    hr = dxCommon_->GetDevice()->CreateComputePipelineState(
        &desc,
        IID_PPV_ARGS(&initializePipelineState_)
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
    commandList->SetComputeRootConstantBufferView(
        1,
        updateBuffer_->GetGPUVirtualAddress()
    );
    commandList->Dispatch(
        (kMaxParticles + kThreadCount - 1) / kThreadCount,
        1,
        1
    );

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = particleBuffer_.Get();
    commandList->ResourceBarrier(1, &barrier);

}

void GPUParticleManager::UploadPendingParticles()
{
    if (pendingIndices_.empty()) {
        return;
    }

    TransitionParticleResource(
        D3D12_RESOURCE_STATE_COPY_DEST
    );

    ID3D12GraphicsCommandList* commandList =
        dxCommon_->GetCommandList();

    for (uint32_t index : pendingIndices_) {
        const UINT64 offset =
            sizeof(ParticleData) * static_cast<UINT64>(index);

        commandList->CopyBufferRegion(
            particleBuffer_.Get(),
            offset,
            uploadBuffer_.Get(),
            offset,
            sizeof(ParticleData)
        );
    }

    pendingIndices_.clear();
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
