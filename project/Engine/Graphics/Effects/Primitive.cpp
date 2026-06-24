#include "Primitive.h"

#include <cassert>
#include <cstring>
#include <numbers>
#include <random>

#include <algorithm>
#include <cmath>

#include "D3DResourceHelper.h"
#include "EffectMath.h"

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


void Primitive::Update(
    const Matrix4x4& viewMatrix,
    const Matrix4x4& projectionMatrix
) {
    constexpr float deltaTime =
        1.0f / 60.0f;

    for (
        auto iterator = particles_.begin();
        iterator != particles_.end();
        ) {
        Particle& particle =
            *iterator;

        particle.lifeTime +=
            deltaTime;

        if (
            particle.lifeTime >=
            particle.maxTime
            ) {
            iterator =
                particles_.erase(iterator);

            continue;
        }

        const Settings& settings =
            particle.settings;

        // 加速度
        particle.velocity.x +=
            settings.acceleration.x *
            deltaTime;

        particle.velocity.y +=
            settings.acceleration.y *
            deltaTime;

        particle.velocity.z +=
            settings.acceleration.z *
            deltaTime;

        // 移動
        particle.transform.translate.x +=
            particle.velocity.x *
            deltaTime;

        particle.transform.translate.y +=
            particle.velocity.y *
            deltaTime;

        particle.transform.translate.z +=
            particle.velocity.z *
            deltaTime;

        particle.transform.rotate.z +=
            particle.angularVelocity *
            deltaTime;

        const float time =
            std::clamp(
                particle.lifeTime /
                particle.maxTime,
                0.0f,
                1.0f
            );

        const float scaleTime =
            EffectMath::EaseOut(
                time,
                settings.scaleEasePower
            );

        particle.transform.scale.x =
            particle.initialScale.x *
            (
                1.0f +
                (
                    settings.endWidthScale -
                    1.0f
                    ) *
                scaleTime
                );

        particle.transform.scale.y =
            particle.initialScale.y *
            (
                1.0f +
                (
                    settings.endLengthScale -
                    1.0f
                    ) *
                scaleTime
                );

        // 開始色から終了色へ補間
        const float fadeIn =
            EffectMath::FadeIn(
                time,
                settings.fadeInRatio
            );

        const float fadeOut =
            EffectMath::FadeOut(
                time,
                settings.fadePower
            );

        particle.color =
            EffectMath::MakeFadedColor(
                settings.color,
                settings.endColor,
                time,
                settings.intensity,
                fadeIn,
                fadeOut
            );

        ++iterator;
    }

    uint32_t index = 0;

    Matrix4x4 billboardMatrix =
        Math::MakeBillboardMatrix(viewMatrix);

    const Matrix4x4 viewProjectionMatrix =
        Math::Multiply(
            viewMatrix,
            projectionMatrix
        );

    for (const Particle& particle : particles_) {
        if (index >= kMaxParticles) {
            break;
        }

        const Matrix4x4 scaleMatrix =
            Math::Matrix4x4MakeScaleMatrix(
                particle.transform.scale
            );

        const Matrix4x4 rotateMatrix =
            Math::MakeRotateZMatrix(
                particle.transform.rotate.z
            );

        const Matrix4x4 translateMatrix =
            Math::MakeTranslateMatrix(
                particle.transform.translate
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
        (std::max)(
            0.001f,
            (std::min)(
                settings_.minLength,
                settings_.maxLength
                )
            );

    const float maximumLength =
        (std::max)(
            minimumLength,
            (std::max)(
                settings_.minLength,
                settings_.maxLength
                )
            );

    const float minimumLifeTime =
        (std::max)(
            0.001f,
            (std::min)(
                settings_.minLifeTime,
                settings_.maxLifeTime
                )
            );

    const float maximumLifeTime =
        (std::max)(
            minimumLifeTime,
            (std::max)(
                settings_.minLifeTime,
                settings_.maxLifeTime
                )
            );

    const float spread =
        std::clamp(
            settings_.directionSpread,
            0.0f,
            std::numbers::pi_v<float>
        );

    std::uniform_real_distribution<float>
        angleDistribution(
            settings_.directionAngle - spread,
            settings_.directionAngle + spread
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

    std::uniform_real_distribution<float>
        unitDistribution(
            0.0f,
            1.0f
        );

    std::uniform_real_distribution<float>
        signedDistribution(
            -1.0f,
            1.0f
        );

    for (
        uint32_t index = 0;
        index < count;
        ++index
        ) {
        if (
            particles_.size() >=
            kMaxParticles
            ) {
            return;
        }

        Particle particle{};

        // 発生時の設定をコピー
        particle.settings =
            settings_;

        const float angle =
            angleDistribution(
                primitiveRandomEngine
            );

        const float widthScale =
            (std::max)(
                0.01f,
                1.0f +
                signedDistribution(
                    primitiveRandomEngine
                ) *
                settings_.widthRandomness
                );

        particle.transform.scale = {
            settings_.width * widthScale,
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

        const float spawnAngle =
            unitDistribution(
                primitiveRandomEngine
            ) *
            std::numbers::pi_v<float> *
            2.0f;

        const float spawnDistance =
            std::sqrt(
                unitDistribution(
                    primitiveRandomEngine
                )
            ) *
            (std::max)(
                settings_.spawnRadius,
                0.0f
                );

        particle.transform.translate = {
            position.x +
                std::cos(spawnAngle) *
                spawnDistance,

            position.y +
                std::sin(spawnAngle) *
                spawnDistance,

            position.z
        };

        const float speedScale =
            (std::max)(
                0.0f,
                1.0f +
                signedDistribution(
                    primitiveRandomEngine
                ) *
                settings_.moveSpeedRandomness
                );

        const float speed =
            settings_.moveSpeed *
            speedScale;

        particle.velocity = {
            -std::sin(angle) * speed,
            std::cos(angle) * speed,
            0.0f
        };

        particle.angularVelocity =
            settings_.rotationSpeed +
            signedDistribution(
                primitiveRandomEngine
            ) *
            settings_.rotationSpeedRandomness;

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

    D3DResourceHelper::SetParticlePipelineDefaults(description);

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

    vertexBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            dxCommon_->GetDevice(),
            bufferSize
        );

    VertexData* mappedData =
        D3DResourceHelper::Map<VertexData>(
            vertexBuffer_.Get()
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
