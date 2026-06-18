#include "Ring.h"
#include <cassert>
#include <cmath>
#include <numbers>
#include <vector>
#include <cstring>
#include <algorithm>

using namespace Microsoft::WRL;

void Ring::SetThickness(float thickness)
{
    settings_.thickness =
        std::clamp(
            thickness,
            0.001f,
            0.5f
        );
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
    constexpr float deltaTime =
        1.0f / 60.0f;

    for (
        auto iterator = rings_.begin();
        iterator != rings_.end();
        ) {
        RingParticle& ring =
            *iterator;

        ring.lifeTime +=
            deltaTime;

        if (ring.lifeTime >= ring.maxTime) {
            iterator =
                rings_.erase(iterator);

            continue;
        }

        const Settings& settings =
            ring.settings;

        const float time =
            std::clamp(
                ring.lifeTime /
                ring.maxTime,
                0.0f,
                1.0f
            );

        const float easedTime =
            1.0f -
            std::pow(
                1.0f - time,
                (std::max)(
                    settings.easePower,
                    0.01f
                )
            );

        const float scale =
            settings.startScale +
            (
                settings.endScale -
                settings.startScale
                ) *
            easedTime;

        ring.transform.scale = {
            scale,
            scale,
            1.0f
        };

        ring.transform.rotate.z +=
            settings.rotationSpeed *
            deltaTime;

        ring.currentThickness =
            settings.thickness +
            (
                settings.endThickness -
                settings.thickness
                ) *
            easedTime;

        ring.currentThickness =
            std::clamp(
                ring.currentThickness,
                0.001f,
                0.5f
            );

        ring.distortionPhase +=
            settings.distortionSpeed *
            deltaTime;

        float fadeIn = 1.0f;

        if (settings.fadeInRatio > 0.0f) {
            fadeIn =
                std::clamp(
                    time /
                    settings.fadeInRatio,
                    0.0f,
                    1.0f
                );
        }

        const float fadeOut =
            (1.0f - time) *
            (1.0f - time);

        const float red =
            settings.color.x +
            (
                settings.endColor.x -
                settings.color.x
                ) *
            time;

        const float green =
            settings.color.y +
            (
                settings.endColor.y -
                settings.color.y
                ) *
            time;

        const float blue =
            settings.color.z +
            (
                settings.endColor.z -
                settings.color.z
                ) *
            time;

        const float alpha =
            settings.color.w +
            (
                settings.endColor.w -
                settings.color.w
                ) *
            time;

        ring.color = {
            red * settings.intensity,
            green * settings.intensity,
            blue * settings.intensity,
            alpha * fadeIn * fadeOut
        };

        ++iterator;
    }

    const Matrix4x4 cameraMatrix =
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

    for (const RingParticle& ring : rings_) {
        if (index >= kMaxRings) {
            break;
        }

        const Matrix4x4 scaleMatrix =
            Math::Matrix4x4MakeScaleMatrix(
                ring.transform.scale
            );

        const Matrix4x4 rotateMatrix =
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
                    rotateMatrix
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

        instancingDataMapped_[index].parameters0 = {
            ring.currentThickness,
            ring.settings.distortionStrength,
            ring.settings.distortionFrequency,
            ring.distortionPhase
        };

        instancingDataMapped_[index].parameters1 = {
            ring.settings.edgeSoftness,
            ring.settings.glowStrength,
            0.0f,
            0.0f
        };

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

    // 発生時設定を保存
    ring.settings =
        settings_;

    ring.transform.translate =
        position;

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

    ring.color =
        settings_.color;

    ring.currentThickness =
        settings_.thickness;

    ring.distortionPhase = 0.0f;

    ring.lifeTime = 0.0f;

    ring.maxTime =
        (std::max)(
            settings_.lifeTime,
            0.001f
        );

    rings_.push_back(ring);
}


void Ring::CreateRootSignature()
{

    D3D12_DESCRIPTOR_RANGE descriptorRange{};
    descriptorRange.RangeType =
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

    descriptorRange.NumDescriptors = 1;
    descriptorRange.BaseShaderRegister = 0;
    descriptorRange.RegisterSpace = 0;

    descriptorRange.OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameter{};
    rootParameter.ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

    rootParameter.DescriptorTable.NumDescriptorRanges =
        1;

    rootParameter.DescriptorTable.pDescriptorRanges =
        &descriptorRange;

    rootParameter.ShaderVisibility =
        D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC staticSampler{};
    staticSampler.Filter =
        D3D12_FILTER_MIN_MAG_MIP_LINEAR;

    staticSampler.AddressU =
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    staticSampler.AddressV =
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    staticSampler.AddressW =
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    staticSampler.MipLODBias = 0.0f;
    staticSampler.MaxAnisotropy = 1;

    staticSampler.ComparisonFunc =
        D3D12_COMPARISON_FUNC_NEVER;

    staticSampler.BorderColor =
        D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;

    staticSampler.MinLOD = 0.0f;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;

    staticSampler.ShaderRegister = 0;
    staticSampler.RegisterSpace = 0;

    staticSampler.ShaderVisibility =
        D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC description{};
    description.NumParameters = 1;
    description.pParameters =
        &rootParameter;

    description.NumStaticSamplers = 1;
    description.pStaticSamplers =
        &staticSampler;

    description.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob>
        signatureBlob;

    Microsoft::WRL::ComPtr<ID3DBlob>
        errorBlob;

    HRESULT result =
        D3D12SerializeRootSignature(
            &description,
            D3D_ROOT_SIGNATURE_VERSION_1,
            &signatureBlob,
            &errorBlob
        );

    if (FAILED(result)) {
        if (errorBlob) {
            OutputDebugStringA(
                static_cast<const char*>(
                    errorBlob->GetBufferPointer()
                    )
            );
        }

        assert(false);
        return;
    }

    result =
        dxCommon_->GetDevice()
        ->CreateRootSignature(
            0,
            signatureBlob->GetBufferPointer(),
            signatureBlob->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature_)
        );

    assert(SUCCEEDED(result));

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
        { "INSTANCE_PARAMETERS", 0 , DXGI_FORMAT_R32G32B32A32_FLOAT ,1,80,D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,1},
        {"INSTANCE_PARAMETERS",1,DXGI_FORMAT_R32G32B32A32_FLOAT,1,96,D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,1}
    };

    auto vsBlob = dxCommon_->CompileShader(
        L"Resources/shaders/hlsl/Ring.VS.hlsl",
        L"vs_6_0"
    );

    auto psBlob = dxCommon_->CompileShader(
        L"Resources/shaders/hlsl/Ring.PS.hlsl",
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
    const VertexData vertices[] = {
        {
            {-1.0f,  1.0f, 0.0f, 1.0f},
            {0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            { 1.0f,  1.0f, 0.0f, 1.0f},
            {1.0f, 0.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            {-1.0f, -1.0f, 0.0f, 1.0f},
            {0.0f, 1.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            {-1.0f, -1.0f, 0.0f, 1.0f},
            {0.0f, 1.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            { 1.0f,  1.0f, 0.0f, 1.0f},
            {1.0f, 0.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            { 1.0f, -1.0f, 0.0f, 1.0f},
            {1.0f, 1.0f},
            {0.0f, 0.0f, -1.0f}
        }
    };

    vertexCount_ =
        static_cast<uint32_t>(
            std::size(vertices)
            );

    const UINT bufferSize =
        sizeof(vertices);

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

    resourceDescription.Width =
        bufferSize;

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
        reinterpret_cast<void**>(
            &mappedData
            )
    );

    std::memcpy(
        mappedData,
        vertices,
        bufferSize
    );

    vertexBuffer_->Unmap(
        0,
        nullptr
    );

    vertexBufferView_.BufferLocation =
        vertexBuffer_
        ->GetGPUVirtualAddress();

    vertexBufferView_.SizeInBytes =
        bufferSize;

    vertexBufferView_.StrideInBytes =
        sizeof(VertexData);
}