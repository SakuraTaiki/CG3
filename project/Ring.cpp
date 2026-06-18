#include "Ring.h"
#include <cassert>
#include <cmath>
#include <numbers>
#include <vector>
#include <cstring>

using namespace Microsoft::WRL;

void Ring::SetThickness(float thickness)
{

    if (thickness < 0.02f) {
        thickness = 0.02f;
    }

    if (thickness > 0.95f) {
        thickness = 0.95f;
    }

    if (settings_.thickness != thickness) {
        settings_.thickness = thickness;
        meshDirty_ = true;
    }

}

void Ring::Initialize(DirectXCommon* dxCommon, TextureManager* textureManager) {
    assert(dxCommon);
    assert(textureManager);

    dxCommon_ = dxCommon;
    textureManager_ = textureManager;

    textureHandle_ = textureManager_->LoadTexture("Resources/white.png");

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

void Ring::Update(
    const Matrix4x4& viewMatrix,
    const Matrix4x4& projectionMatrix
) {
    // ImGuiで太さが変更された場合だけ再作成
    if (meshDirty_) {
        CreateMesh();
        meshDirty_ = false;
    }

    for (
        auto iterator = rings_.begin();
        iterator != rings_.end();
        ) {
        iterator->lifeTime += 1.0f / 60.0f;

        if (iterator->lifeTime >=
            iterator->maxTime) {

            iterator = rings_.erase(iterator);
            continue;
        }

        const float time =
            iterator->lifeTime /
            iterator->maxTime;

        // EaseOut
        const float inverseTime =
            1.0f - time;

        const float easedTime =
            1.0f -
            std::pow(
                inverseTime,
                settings_.easePower
            );

        const float scale =
            settings_.startScale +
            (
                settings_.endScale -
                settings_.startScale
                ) * easedTime;

        iterator->transform.scale = {
            scale,
            scale,
            1.0f
        };

        iterator->transform.rotate.z +=
            settings_.rotationSpeed *
            (1.0f / 60.0f);

        float fadeIn = 1.0f;

        if (settings_.fadeInRatio > 0.0f) {
            fadeIn =
                time /
                settings_.fadeInRatio;

            if (fadeIn > 1.0f) {
                fadeIn = 1.0f;
            }
        }

        const float fadeOut =
            (1.0f - time) *
            (1.0f - time);

        // ImGuiの色を即時反映
        iterator->color.x =
            settings_.color.x *
            settings_.intensity;

        iterator->color.y =
            settings_.color.y *
            settings_.intensity;

        iterator->color.z =
            settings_.color.z *
            settings_.intensity;

        iterator->color.w =
            settings_.color.w *
            fadeIn *
            fadeOut;

        ++iterator;
    }

    Matrix4x4 cameraMatrix =
        Math::Inverse(viewMatrix);

    Matrix4x4 billboardMatrix =
        Math::MakeIdentity4x4();

    billboardMatrix.m[0][0] =
        cameraMatrix.m[0][0];
    billboardMatrix.m[0][1] =
        cameraMatrix.m[0][1];
    billboardMatrix.m[0][2] =
        cameraMatrix.m[0][2];

    billboardMatrix.m[1][0] =
        cameraMatrix.m[1][0];
    billboardMatrix.m[1][1] =
        cameraMatrix.m[1][1];
    billboardMatrix.m[1][2] =
        cameraMatrix.m[1][2];

    billboardMatrix.m[2][0] =
        cameraMatrix.m[2][0];
    billboardMatrix.m[2][1] =
        cameraMatrix.m[2][1];
    billboardMatrix.m[2][2] =
        cameraMatrix.m[2][2];

    const Matrix4x4 viewProjectionMatrix =
        Math::Multiply(
            viewMatrix,
            projectionMatrix
        );

    uint32_t index = 0;

    for (const auto& ring : rings_) {
        if (index >= kMaxRings) {
            break;
        }

        const Matrix4x4 scaleMatrix =
            Math::Matrix4x4MakeScaleMatrix(
                ring.transform.scale
            );

        const Matrix4x4 rotationMatrix =
            Math::MakeRotateZMatrix(
                ring.transform.rotate.z
            );

        const Matrix4x4 translateMatrix =
            Math::MakeTranslateMatrix(
                ring.transform.translate
            );

        const Matrix4x4 worldMatrix =
            Math::Multiply(
                Math::Multiply(
                    scaleMatrix,
                    rotationMatrix
                ),
                Math::Multiply(
                    billboardMatrix,
                    translateMatrix
                )
            );

        instancingDataMapped_[index].WVP =
            Math::Multiply(
                worldMatrix,
                viewProjectionMatrix
            );

        instancingDataMapped_[index].color =
            ring.color;

        ++index;
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
    if (!isActive_) {
        return;
    }

    if (rings_.size() >= kMaxRings) {
        return;
    }

    RingParticle ring{};

    ring.transform.translate = position;

    ring.transform.scale = {
        settings_.startScale,
        settings_.startScale,
        1.0f
    };

    ring.transform.rotate = {
        0.0f,
        0.0f,
        0.0f
    };

    ring.color = settings_.color;

    ring.lifeTime = 0.0f;
    ring.maxTime = settings_.lifeTime;

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

void Ring::CreateMesh()
{
    const uint32_t kRingDivide = 64;

    const float outerRadius = 1.0f;

    const float innerRadius =
        outerRadius - settings_.thickness;

    const float radianPerDivide =
        2.0f *
        std::numbers::pi_v<float> /
        static_cast<float>(kRingDivide);

    std::vector<VertexData> vertices;
    vertices.reserve(kRingDivide * 6);

    for (
        uint32_t index = 0;
        index < kRingDivide;
        ++index
        ) {
        const float currentRadian =
            index * radianPerDivide;

        const float nextRadian =
            (index + 1) * radianPerDivide;

        const float currentSin =
            std::sin(currentRadian);

        const float currentCos =
            std::cos(currentRadian);

        const float nextSin =
            std::sin(nextRadian);

        const float nextCos =
            std::cos(nextRadian);

        const float currentU =
            static_cast<float>(index) /
            static_cast<float>(kRingDivide);

        const float nextU =
            static_cast<float>(index + 1) /
            static_cast<float>(kRingDivide);

        VertexData outerCurrent = {
            {
                -currentSin * outerRadius,
                currentCos * outerRadius,
                0.0f,
                1.0f
            },
            {currentU, 0.0f},
            {0.0f, 0.0f, -1.0f}
        };

        VertexData outerNext = {
            {
                -nextSin * outerRadius,
                nextCos * outerRadius,
                0.0f,
                1.0f
            },
            {nextU, 0.0f},
            {0.0f, 0.0f, -1.0f}
        };

        VertexData innerCurrent = {
            {
                -currentSin * innerRadius,
                currentCos * innerRadius,
                0.0f,
                1.0f
            },
            {currentU, 1.0f},
            {0.0f, 0.0f, -1.0f}
        };

        VertexData innerNext = {
            {
                -nextSin * innerRadius,
                nextCos * innerRadius,
                0.0f,
                1.0f
            },
            {nextU, 1.0f},
            {0.0f, 0.0f, -1.0f}
        };

        vertices.push_back(outerCurrent);
        vertices.push_back(outerNext);
        vertices.push_back(innerCurrent);

        vertices.push_back(innerCurrent);
        vertices.push_back(outerNext);
        vertices.push_back(innerNext);
    }

    vertexCount_ =
        static_cast<uint32_t>(
            vertices.size()
            );

    const UINT bufferSize =
        static_cast<UINT>(
            sizeof(VertexData) *
            vertices.size()
            );

    D3D12_HEAP_PROPERTIES heapProperties = {
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        1,
        1
    };

    D3D12_RESOURCE_DESC resourceDescription{};
    resourceDescription.Dimension =
        D3D12_RESOURCE_DIMENSION_BUFFER;

    resourceDescription.Width = bufferSize;
    resourceDescription.Height = 1;
    resourceDescription.DepthOrArraySize = 1;
    resourceDescription.MipLevels = 1;

    resourceDescription.Layout =
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    resourceDescription.SampleDesc.Count = 1;

    HRESULT result =
        dxCommon_->GetDevice()
        ->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDescription,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vertexBuffer_)
        );

    assert(SUCCEEDED(result));

    VertexData* mappedData = nullptr;

    vertexBuffer_->Map(
        0,
        nullptr,
        reinterpret_cast<void**>(&mappedData)
    );

    std::memcpy(
        mappedData,
        vertices.data(),
        bufferSize
    );

    vertexBuffer_->Unmap(
        0,
        nullptr
    );

    vertexBufferView_.BufferLocation =
        vertexBuffer_->GetGPUVirtualAddress();

    vertexBufferView_.SizeInBytes =
        bufferSize;

    vertexBufferView_.StrideInBytes =
        sizeof(VertexData);
}