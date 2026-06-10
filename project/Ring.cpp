#include "Ring.h"
#include <cassert>
#include <cmath>
#include <numbers>
#include <vector>
#include <cstring>

using namespace Microsoft::WRL;

void Ring::Initialize(DirectXCommon* dxCommon, TextureManager* textureManager) {
    assert(dxCommon);
    assert(textureManager);

    dxCommon_ = dxCommon;
    textureManager_ = textureManager;

    textureHandle_ = textureManager_->LoadTexture("Resources/uvChecker.png");

    CreateRootSignature();
    CreatePipelineState();
    CreateMesh();

    UINT size = sizeof(InstanceData)*kMaxRings;

    D3D12_HEAP_PROPERTIES heapProps = {
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        1,
        1
    };

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = size;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resDesc.SampleDesc.Count = 1;

    HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&instancingBuffer_)
    );
    assert(SUCCEEDED(hr));

    instancingBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&instancingDataMapped_));

    instancingBufferView_.BufferLocation = instancingBuffer_->GetGPUVirtualAddress();
    instancingBufferView_.SizeInBytes = size;
    instancingBufferView_.StrideInBytes = sizeof(InstanceData);
}

void Ring::Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {

    for (auto it = rings_.begin(); it != rings_.end();) {
        it->lifeTime += 1.0f / 60.0f;
        if (it->lifeTime >= it->maxTime) {
            it = rings_.erase(it);
            continue;
        }
        float t = it->lifeTime / it->maxTime;

        //時間でRingを広げる
        float scale = 0.2f + t * expandSpeed_;
        it->transform.scale = { scale, scale, 1.0f };

        // 徐々に消す
        it->color.w = 1.0f - t;

        ++it;
    }

    Matrix4x4 cameraMatrix = Math::Inverse(viewMatrix);

    Matrix4x4 billboardMat = Math::MakeIdentity4x4();
    billboardMat.m[0][0] = cameraMatrix.m[0][0];
    billboardMat.m[0][1] = cameraMatrix.m[0][1];
    billboardMat.m[0][2] = cameraMatrix.m[0][2];

    billboardMat.m[1][0] = cameraMatrix.m[1][0];
    billboardMat.m[1][1] = cameraMatrix.m[1][1];
    billboardMat.m[1][2] = cameraMatrix.m[1][2];

    billboardMat.m[2][0] = cameraMatrix.m[2][0];
    billboardMat.m[2][1] = cameraMatrix.m[2][1];
    billboardMat.m[2][2] = cameraMatrix.m[2][2];

    uint32_t index = 0;

    for (const auto& ring : rings_) {
        if (index >= kMaxRings) {
            break;
        }

        Matrix4x4 scaleMat = Math::Matrix4x4MakeScaleMatrix(scale_);
        Matrix4x4 transMat = Math::MakeTranslateMatrix(position_);

        Matrix4x4 worldMat =
            Math::Multiply(
                scaleMat,
                Math::Multiply(billboardMat, transMat)
            );

        Matrix4x4 wvp =
            Math::Multiply(worldMat, Math::Multiply(viewMatrix, projectionMatrix));

        instancingDataMapped_[index].WVP = wvp;
        instancingDataMapped_[index].color = color_;
        index++;
    }
}

void Ring::Draw() {
    if (!isActive_) {
        return;
    }

    auto commandList = dxCommon_->GetCommandList();

    commandList->SetPipelineState(pipelineState_.Get());
    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetVertexBuffers(1, 1, &instancingBufferView_);

    auto srvHandle = textureManager_->GetSrvHandleGPU(textureHandle_);
    commandList->SetGraphicsRootDescriptorTable(0, srvHandle);

    if (rings_.empty()) {
        return;
    }

    uint32_t count = static_cast<uint32_t>(rings_.size());
    commandList->DrawInstanced(vertexCount_, count, 0, 0);
}

void Ring::Emit(const Vector3& position)
{
    if (rings_.size() >= kMaxRings) {
        return;
    }

    RingParticle ring;

    ring.transform.translate = position;
    ring.transform.scale = { 0.2f,0.2f,1.0f };
    ring.transform.rotate = { 0.0f,0.0f,0.0f };

    ring.color = { 1.0f, 1.0f, 1.0f, 0.8f };

    ring.lifeTime = 0.0f;
    ring.maxTime = 0.5f;

    rings_.push_back(ring);
}

void Ring::CreateRootSignature() {
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = 0;

    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParam.DescriptorTable.NumDescriptorRanges = 1;
    rootParam.DescriptorTable.pDescriptorRanges = &range;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = 1;
    desc.pParameters = &rootParam;
    desc.NumStaticSamplers = 1;
    desc.pStaticSamplers = &sampler;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

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
        IID_PPV_ARGS(&rootSignature_)
    );
    assert(SUCCEEDED(hr));
}

void Ring::CreatePipelineState() {
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "INSTANCE_WVP", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_WVP", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_WVP", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_WVP", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },

        { "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    };

    auto vsBlob = dxCommon_->CompileShader(
        L"Resources/shaders/hlsl/Particle.VS.hlsl",
        L"vs_6_0"
    );

    auto psBlob = dxCommon_->CompileShader(
        L"Resources/shaders/hlsl/Particle.PS.hlsl",
        L"ps_6_0"
    );

    assert(vsBlob);
    assert(psBlob);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

    auto& blend = psoDesc.BlendState.RenderTarget[0];
    blend.BlendEnable = TRUE;
    blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend.DestBlend = D3D12_BLEND_ONE;
    blend.BlendOp = D3D12_BLEND_OP_ADD;
    blend.SrcBlendAlpha = D3D12_BLEND_ONE;
    blend.DestBlendAlpha = D3D12_BLEND_ZERO;
    blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc.Count = 1;

    HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&pipelineState_)
    );
    assert(SUCCEEDED(hr));
}

void Ring::CreateMesh() {
    const uint32_t kRingDivide = 32;
    const float kOuterRadius = 1.0f;
    const float kInnerRadius = 0.5f;
    const float radianPerDivide =
        2.0f * std::numbers::pi_v<float> / float(kRingDivide);

    std::vector<VertexData> vertices;
    vertices.reserve(kRingDivide * 6);

    for (uint32_t index = 0; index < kRingDivide; ++index) {
        float sin = std::sin(index * radianPerDivide);
        float cos = std::cos(index * radianPerDivide);
        float sinNext = std::sin((index + 1) * radianPerDivide);
        float cosNext = std::cos((index + 1) * radianPerDivide);

        float u = float(index) / float(kRingDivide);
        float uNext = float(index + 1) / float(kRingDivide);

        VertexData v0 = {
            { -sin * kOuterRadius, cos * kOuterRadius, 0.0f, 1.0f },
            { u, 0.0f },
            { 0.0f, 0.0f, -1.0f }
        };

        VertexData v1 = {
            { -sinNext * kOuterRadius, cosNext * kOuterRadius, 0.0f, 1.0f },
            { uNext, 0.0f },
            { 0.0f, 0.0f, -1.0f }
        };

        VertexData v2 = {
            { -sin * kInnerRadius, cos * kInnerRadius, 0.0f, 1.0f },
            { u, 1.0f },
            { 0.0f, 0.0f, -1.0f }
        };

        VertexData v3 = {
            { -sinNext * kInnerRadius, cosNext * kInnerRadius, 0.0f, 1.0f },
            { uNext, 1.0f },
            { 0.0f, 0.0f, -1.0f }
        };

        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);

        vertices.push_back(v2);
        vertices.push_back(v1);
        vertices.push_back(v3);
    }

    vertexCount_ = static_cast<uint32_t>(vertices.size());

    UINT size = UINT(sizeof(VertexData) * vertices.size());

    D3D12_HEAP_PROPERTIES heapProps = {
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        1,
        1
    };

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = size;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resDesc.SampleDesc.Count = 1;

    HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexBuffer_)
    );
    assert(SUCCEEDED(hr));

    VertexData* data = nullptr;
    vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&data));
    memcpy(data, vertices.data(), size);
    vertexBuffer_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = size;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
}
