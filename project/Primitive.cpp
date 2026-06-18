#include "Primitive.h"

#include <cassert>
#include <cstring>
#include <numbers>
#include <random>

#include <algorithm>
#include <cmath>

using namespace Microsoft::WRL;

namespace
{
    std::random_device primitiveSeedGenerator;

    std::mt19937_64 primitiveRandomEngine(
        primitiveSeedGenerator()
    );
}

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

    // 配布された縦長の光テクスチャ
    textureHandle_ =
        textureManager_->LoadTexture(
            "Resources/white.png"
        );

    CreateRootSignature();
    CreatePipelineState();
    CreateMesh();

    const UINT bufferSize =
        sizeof(InstanceData) * kMaxParticles;

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
            IID_PPV_ARGS(&instancingBuffer_)
        );

    assert(SUCCEEDED(result));

    instancingBuffer_->Map(
        0,
        nullptr,
        reinterpret_cast<void**>(
            &instancingDataMapped_
            )
    );

    instancingBufferView_.BufferLocation =
        instancingBuffer_
        ->GetGPUVirtualAddress();

    instancingBufferView_.SizeInBytes =
        bufferSize;

    instancingBufferView_.StrideInBytes =
        sizeof(InstanceData);
}

void Primitive::Update(
    const Matrix4x4& viewMatrix,
    const Matrix4x4& projectionMatrix
) {
    const float deltaTime = 1.0f / 60.0f;

    for (
        auto iterator = particles_.begin();
        iterator != particles_.end();
        ) {
        iterator->lifeTime += deltaTime;

        if (iterator->lifeTime >=
            iterator->maxTime) {

            iterator =
                particles_.erase(iterator);

            continue;
        }

        iterator->transform.translate.x +=
            iterator->velocity.x * deltaTime;

        iterator->transform.translate.y +=
            iterator->velocity.y * deltaTime;

        iterator->transform.translate.z +=
            iterator->velocity.z * deltaTime;

        iterator->transform.rotate.z +=
            iterator->angularVelocity *
            deltaTime;

        const float time =
            iterator->lifeTime /
            iterator->maxTime;

        iterator->transform.scale.x =
            iterator->initialScale.x *
            (
                1.0f +
                (
                    settings_.endWidthScale -
                    1.0f
                    ) * time
                );

        iterator->transform.scale.y =
            iterator->initialScale.y *
            (
                1.0f +
                (
                    settings_.endLengthScale -
                    1.0f
                    ) * time
                );

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
            std::pow(
                1.0f - time,
                settings_.fadePower
            );

        ++iterator;
    }

    uint32_t index = 0;

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

    for (const auto& particle : particles_) {
        if (index >= kMaxParticles) {
            break;
        }

        Matrix4x4 scaleMatrix =
            Math::Matrix4x4MakeScaleMatrix(
                particle.transform.scale
            );

        Matrix4x4 rotateMatrix =
            Math::MakeRotateZMatrix(
                particle.transform.rotate.z
            );

        Matrix4x4 translateMatrix =
            Math::MakeTranslateMatrix(
                particle.transform.translate
            );

        Matrix4x4 worldMatrix =
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

        Matrix4x4 viewProjectionMatrix =
            Math::Multiply(
                viewMatrix,
                projectionMatrix
            );

        Matrix4x4 wvp =
            Math::Multiply(
                worldMatrix,
                viewProjectionMatrix
            );

        instancingDataMapped_[index].WVP =
            wvp;

        instancingDataMapped_[index].color =
            particle.color;

        ++index;
    }
}

void Primitive::Draw()
{
    if (!isActive_) {
        return;
    }

    if (particles_.empty()) {
        return;
    }

    auto commandList =
        dxCommon_->GetCommandList();

    assert(
        pipelineState_ != nullptr &&
        "Primitive PipelineState not created!"
    );

    commandList->SetPipelineState(
        pipelineState_.Get()
    );

    commandList->SetGraphicsRootSignature(
        rootSignature_.Get()
    );

    commandList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );

    commandList->IASetVertexBuffers(
        0,
        1,
        &vertexBufferView_
    );

    commandList->IASetVertexBuffers(
        1,
        1,
        &instancingBufferView_
    );

    auto textureHandle =
        textureManager_->GetSrvHandleGPU(
            textureHandle_
        );

    commandList->SetGraphicsRootDescriptorTable(
        0,
        textureHandle
    );

    uint32_t particleCount =
        static_cast<uint32_t>(
            particles_.size()
            );

    if (particleCount > kMaxParticles) {
        particleCount = kMaxParticles;
    }

    commandList->DrawInstanced(
        6,
        particleCount,
        0,
        0
    );
}


void Primitive::Emit(
    const Vector3& position,
    uint32_t count
) {
    if (!isActive_) {
        return;
    }

    const float minimumLength =
        (std::min)(
            settings_.minLength,
            settings_.maxLength
            );

    const float maximumLength =
        (std::max)(
            settings_.minLength,
            settings_.maxLength
            );

    const float minimumLifeTime =
        (std::min)(
            settings_.minLifeTime,
            settings_.maxLifeTime
            );

    const float maximumLifeTime =
        (std::max)(
            settings_.minLifeTime,
            settings_.maxLifeTime
            );

    std::uniform_real_distribution<float>
        rotationDistribution(
            -std::numbers::pi_v<float>,
            std::numbers::pi_v<float>
        );

    std::uniform_real_distribution<float>
        lengthDistribution(
            minimumLength,
            maximumLength
        );

    std::uniform_real_distribution<float>
        lifeTimeDistribution(
            minimumLifeTime,
            maximumLifeTime
        );

    for (
        uint32_t index = 0;
        index < count;
        ++index
        ) {
        if (particles_.size() >=
            kMaxParticles) {
            return;
        }

        Particle particle{};

        const float angle =
            rotationDistribution(
                primitiveRandomEngine
            );

        particle.transform.scale = {
            settings_.width,
            lengthDistribution(
                primitiveRandomEngine
            ),
            1.0f
        };

        particle.initialScale =
            particle.transform.scale;

        particle.transform.rotate = {
            0.0f,
            0.0f,
            angle
        };

        particle.transform.translate =
            position;

        // Primitiveが向いている方向へ移動
        particle.velocity = {
            -std::sin(angle) *
                settings_.moveSpeed,

            std::cos(angle) *
                settings_.moveSpeed,

            0.0f
        };

        particle.angularVelocity =
            settings_.rotationSpeed;

        particle.color =
            settings_.color;

        particle.lifeTime = 0.0f;

        particle.maxTime =
            lifeTimeDistribution(
                primitiveRandomEngine
            );

        particles_.push_back(particle);
    }
}

void Primitive::CreateRootSignature()
{
    D3D12_DESCRIPTOR_RANGE descriptorRange{};
    descriptorRange.RangeType =
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

    descriptorRange.NumDescriptors = 1;
    descriptorRange.BaseShaderRegister = 0;

    D3D12_ROOT_PARAMETER rootParameter{};
    rootParameter.ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

    rootParameter
        .DescriptorTable
        .NumDescriptorRanges = 1;

    rootParameter
        .DescriptorTable
        .pDescriptorRanges =
        &descriptorRange;

    rootParameter.ShaderVisibility =
        D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter =
        D3D12_FILTER_MIN_MAG_MIP_LINEAR;

    sampler.AddressU =
        D3D12_TEXTURE_ADDRESS_MODE_WRAP;

    sampler.AddressV =
        D3D12_TEXTURE_ADDRESS_MODE_WRAP;

    sampler.AddressW =
        D3D12_TEXTURE_ADDRESS_MODE_WRAP;

    sampler.ShaderRegister = 0;

    sampler.ShaderVisibility =
        D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC description{};
    description.NumParameters = 1;

    description.pParameters =
        &rootParameter;

    description.NumStaticSamplers = 1;

    description.pStaticSamplers =
        &sampler;

    description.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> errorBlob;

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

void Primitive::CreatePipelineState()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {
            "POSITION",
            0,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        },
        {
            "TEXCOORD",
            0,
            DXGI_FORMAT_R32G32_FLOAT,
            0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        },
        {
            "NORMAL",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        },
        {
            "INSTANCE_WVP",
            0,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            0,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
            1
        },
        {
            "INSTANCE_WVP",
            1,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            16,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
            1
        },
        {
            "INSTANCE_WVP",
            2,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            32,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
            1
        },
        {
            "INSTANCE_WVP",
            3,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            48,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
            1
        },
        {
            "INSTANCE_COLOR",
            0,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            64,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
            1
        }
    };

    auto vertexShader =
        dxCommon_->CompileShader(
            L"Resources/shaders/hlsl/Particle.VS.hlsl",
            L"vs_6_0"
        );

    auto pixelShader =
        dxCommon_->CompileShader(
            L"Resources/shaders/hlsl/Particle.PS.hlsl",
            L"ps_6_0"
        );

    assert(
        vertexShader != nullptr &&
        "Primitive VS compile failed"
    );

    assert(
        pixelShader != nullptr &&
        "Primitive PS compile failed"
    );

    D3D12_GRAPHICS_PIPELINE_STATE_DESC description{};

    description.pRootSignature =
        rootSignature_.Get();

    description.InputLayout = {
        inputLayout,
        _countof(inputLayout)
    };

    description.VS = {
        vertexShader->GetBufferPointer(),
        vertexShader->GetBufferSize()
    };

    description.PS = {
        pixelShader->GetBufferPointer(),
        pixelShader->GetBufferSize()
    };

    auto& blendState =
        description
        .BlendState
        .RenderTarget[0];

    blendState.BlendEnable = TRUE;

    blendState.SrcBlend =
        D3D12_BLEND_SRC_ALPHA;

    blendState.DestBlend =
        D3D12_BLEND_ONE;

    blendState.BlendOp =
        D3D12_BLEND_OP_ADD;

    blendState.SrcBlendAlpha =
        D3D12_BLEND_ONE;

    blendState.DestBlendAlpha =
        D3D12_BLEND_ZERO;

    blendState.BlendOpAlpha =
        D3D12_BLEND_OP_ADD;

    blendState.RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;

    description.RasterizerState.CullMode =
        D3D12_CULL_MODE_NONE;

    description.RasterizerState.FillMode =
        D3D12_FILL_MODE_SOLID;

    description.DepthStencilState.DepthEnable =
        TRUE;

    description.DepthStencilState.DepthWriteMask =
        D3D12_DEPTH_WRITE_MASK_ZERO;

    description.DepthStencilState.DepthFunc =
        D3D12_COMPARISON_FUNC_LESS_EQUAL;

    description.NumRenderTargets = 1;

    description.RTVFormats[0] =
        DXGI_FORMAT_R8G8B8A8_UNORM;

    description.DSVFormat =
        DXGI_FORMAT_D24_UNORM_S8_UINT;

    description.SampleMask =
        D3D12_DEFAULT_SAMPLE_MASK;

    description.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    description.SampleDesc.Count = 1;

    HRESULT result =
        dxCommon_->GetDevice()
        ->CreateGraphicsPipelineState(
            &description,
            IID_PPV_ARGS(&pipelineState_)
        );

    if (FAILED(result)) {
        OutputDebugStringA(
            "Failed to create Primitive PipelineState\n"
        );

        assert(false);
    }
}

void Primitive::CreateMesh()
{
    VertexData vertices[] = {
        {
            {-0.5f, 0.5f, 0.0f, 1.0f},
            {0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            {0.5f, 0.5f, 0.0f, 1.0f},
            {1.0f, 0.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            {-0.5f, -0.5f, 0.0f, 1.0f},
            {0.0f, 1.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            {-0.5f, -0.5f, 0.0f, 1.0f},
            {0.0f, 1.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            {0.5f, 0.5f, 0.0f, 1.0f},
            {1.0f, 0.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            {0.5f, -0.5f, 0.0f, 1.0f},
            {1.0f, 1.0f},
            {0.0f, 0.0f, -1.0f}
        }
    };

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
        vertices,
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