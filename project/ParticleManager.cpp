#include "ParticleManager.h"

#include<cmath>
#include <cassert>
#include <random>
#include<numbers>
#include <algorithm>

using namespace Microsoft::WRL;

// 乱数生成器
static std::random_device seed_gen;
static std::mt19937_64 engine(seed_gen());

void ParticleManager::Initialize(DirectXCommon* dxCommon, TextureManager* textureManager) {
    assert(dxCommon);
    dxCommon_ = dxCommon;
    textureManager_ = textureManager;

    // 1. テクスチャ読み込み
    textureHandle_ = textureManager_->LoadTexture("Resources/uvChecker.png");

    // 2. パイプライン生成
    CreateRootSignature();
    CreatePipelineState();

    // 3. メッシュ生成
    CreateMesh();

    // 4. インスタンシング用バッファ生成
    {
        auto device = dxCommon_->GetDevice();
        UINT size = sizeof(InstanceData) * kMaxParticles;

        D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resDesc.Width = size;
        resDesc.Height = 1; resDesc.DepthOrArraySize = 1; resDesc.MipLevels = 1;
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; resDesc.SampleDesc.Count = 1;

        HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&instancingBuffer_));
        assert(SUCCEEDED(hr));

        instancingBuffer_->Map(0, nullptr, (void**)&instancingDataMapped_);

        instancingBufferView_.BufferLocation = instancingBuffer_->GetGPUVirtualAddress();
        instancingBufferView_.SizeInBytes = size;
        instancingBufferView_.StrideInBytes = sizeof(InstanceData);
    }
}


void ParticleManager::Update(
    const Matrix4x4& viewMatrix,
    const Matrix4x4& projectionMatrix
) {
    constexpr float deltaTime =
        1.0f / 60.0f;

    for (
        auto iterator = particles_.begin();
        iterator != particles_.end();
        ) {
        Particle& particle = *iterator;

        particle.lifeTime += deltaTime;

        if (particle.lifeTime >= particle.maxTime) {
            iterator = particles_.erase(iterator);
            continue;
        }

        particle.velocity.x += particle.acceleration.x;
        particle.velocity.y += particle.acceleration.y;
        particle.velocity.z += particle.acceleration.z;

        particle.transform.translate.x +=
            particle.velocity.x;

        particle.transform.translate.y +=
            particle.velocity.y;

        particle.transform.translate.z +=
            particle.velocity.z;

        const float time =
            particle.lifeTime /
            particle.maxTime;

        if (particle.effectType == 0) {
            particle.transform.scale.x =
                particle.startScale.x *
                (1.0f - time);

            particle.transform.scale.y =
                particle.startScale.y *
                (1.0f - 0.45f * time);

            particle.color.w =
                1.0f - time;
        } else if (particle.effectType == 1) {
            const float scale =
                0.35f + 1.65f * time;

            particle.transform.scale = {
                particle.startScale.x * scale,
                particle.startScale.y * scale,
                1.0f
            };

            particle.color.w =
                (1.0f - time) *
                (1.0f - time);
        } else if (particle.effectType == 2) {
            const float scale =
                0.7f + 0.8f * time;

            particle.transform.scale = {
                particle.startScale.x * scale,
                particle.startScale.y * scale,
                1.0f
            };

            particle.transform.rotate.z +=
                0.015f;

            particle.color.w =
                0.45f * (1.0f - time);
        } else if (particle.effectType == 3) {
            const float pi =
                std::numbers::pi_v<float>;

            const float pulse =
                std::sin(time * pi);

            particle.transform.scale.x =
                particle.startScale.x *
                (0.75f + pulse * 0.45f);

            particle.transform.scale.y =
                particle.startScale.y *
                (0.8f + time * 0.65f);

            particle.transform.rotate.z +=
                0.008f;

            float fadeIn =
                time / 0.1f;

            fadeIn =
                (std::min)(fadeIn, 1.0f);

            const float fadeOut =
                (1.0f - time) *
                (1.0f - time);

            particle.color.w =
                fadeIn * fadeOut;
        } else if (particle.effectType == 4) {
            // 桜の花弁
            particle.transform.rotate.z +=
                particle.angularVelocity;

            const float flutter =
                0.72f +
                std::sin(
                    time * 18.0f +
                    particle.startScale.x * 31.0f
                ) * 0.28f;

            particle.transform.scale.x =
                particle.startScale.x *
                (std::max)(flutter, 0.15f);

            particle.transform.scale.y =
                particle.startScale.y *
                (0.9f + 0.15f * std::sin(time * 12.0f));

            // 横へゆらゆら流す
            particle.transform.translate.x +=
                std::sin(
                    time * 13.0f +
                    particle.transform.rotate.z
                ) * 0.004f;

            const float fadeIn =
                (std::min)(time / 0.08f, 1.0f);

            const float fadeOut =
                (1.0f - time) *
                (1.0f - time);

            particle.color.w =
                fadeIn * fadeOut;
        } else if (particle.effectType == 5) {
            // 桜エフェクトの中心閃光
            const float pulse =
                std::sin(
                    time *
                    std::numbers::pi_v<float>
                );

            const float scale =
                0.25f + pulse * 1.35f;

            particle.transform.scale = {
                particle.startScale.x * scale,
                particle.startScale.y * scale,
                1.0f
            };

            particle.transform.rotate.z +=
                particle.angularVelocity;

            particle.color.w =
                (1.0f - time) *
                (1.0f - time);
        }

        ++iterator;
    }

    uint32_t index = 0;

    const Matrix4x4 cameraMatrix =
        Math::Inverse(viewMatrix);

    Matrix4x4 billboardMatrix =
        Math::MakeIdentity4x4();

    billboardMatrix.m[0][0] = cameraMatrix.m[0][0];
    billboardMatrix.m[0][1] = cameraMatrix.m[0][1];
    billboardMatrix.m[0][2] = cameraMatrix.m[0][2];

    billboardMatrix.m[1][0] = cameraMatrix.m[1][0];
    billboardMatrix.m[1][1] = cameraMatrix.m[1][1];
    billboardMatrix.m[1][2] = cameraMatrix.m[1][2];

    billboardMatrix.m[2][0] = cameraMatrix.m[2][0];
    billboardMatrix.m[2][1] = cameraMatrix.m[2][1];
    billboardMatrix.m[2][2] = cameraMatrix.m[2][2];

    for (const Particle& particle : particles_) {
        if (index >= kMaxParticles) {
            break;
        }

        const Matrix4x4 scaleMatrix =
            Math::Matrix4x4MakeScaleMatrix(
                particle.transform.scale
            );

        const Matrix4x4 rotationMatrix =
            Math::MakeRotateZMatrix(
                particle.transform.rotate.z
            );

        const Matrix4x4 translationMatrix =
            Math::MakeTranslateMatrix(
                particle.transform.translate
            );

        const Matrix4x4 worldMatrix =
            Math::Multiply(
                Math::Multiply(
                    scaleMatrix,
                    rotationMatrix
                ),
                Math::Multiply(
                    billboardMatrix,
                    translationMatrix
                )
            );

        const Matrix4x4 viewProjection =
            Math::Multiply(
                viewMatrix,
                projectionMatrix
            );

        instancingDataMapped_[index].WVP =
            Math::Multiply(
                worldMatrix,
                viewProjection
            );

        instancingDataMapped_[index].color =
            particle.color;

        instancingDataMapped_[index].effectType =
            static_cast<float>(
                particle.effectType
                );

        ++index;
    }
}


void ParticleManager::Draw() {
    if (particles_.empty()) return;

    auto commandList = dxCommon_->GetCommandList();

    // ★ここがNULLだと落ちる。CreatePipelineStateが失敗しているとここでエラーになる
    assert(pipelineState_ != nullptr && "PipelineState not created!");

    commandList->SetPipelineState(pipelineState_.Get());
    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetVertexBuffers(1, 1, &instancingBufferView_);

    auto srvHandle = textureManager_->GetSrvHandleGPU(textureHandle_);
    commandList->SetGraphicsRootDescriptorTable(0, srvHandle);

    uint32_t count = (uint32_t)particles_.size();
    if (count > kMaxParticles) count = kMaxParticles;
    commandList->DrawInstanced(6, count, 0, 0);
}

void ParticleManager::Emit(
    const Vector3& pos,
    uint32_t count
) {
    std::uniform_real_distribution<float> distRotate(
        -std::numbers::pi_v<float>,
        std::numbers::pi_v<float>
    );

    std::uniform_real_distribution<float> distScaleY(
        0.55f,
        1.55f
    );

    std::uniform_real_distribution<float> distSpeed(
        0.035f,
        0.11f
    );

    std::uniform_real_distribution<float> distColor(
        0.75f,
        1.0f
    );

    std::uniform_real_distribution<float> distTime(
        0.22f,
        0.48f
    );

    // 放射状の火花
    for (uint32_t i = 0; i < count; ++i) {
        if (particles_.size() >= kMaxParticles) {
            return;
        }

        Particle particle{};

        particle.transform.scale = {
            0.035f,
            distScaleY(engine),
            1.0f
        };

        particle.startScale = particle.transform.scale;

        particle.transform.rotate = {
            0.0f,
            0.0f,
            distRotate(engine)
        };

        particle.transform.translate = pos;

        const float speed = distSpeed(engine);
        const float rotateZ = particle.transform.rotate.z;

            particle.velocity = {
        -std::sin(rotateZ) *
            speed * 0.55f,

        std::abs(
            std::cos(rotateZ)
        ) * speed + 0.015f,

        0.0f
        };

       
            const float randomColor =
                distColor(engine);

            particle.color = {
                1.0f,
                0.18f + randomColor * 0.45f,
                0.02f,
                1.0f
            };

        particle.lifeTime = 0.0f;
        particle.maxTime = distTime(engine);
        particle.effectType = 0;

        particles_.push_back(particle);
    }

    // 白と青の中心閃光
    for (
        uint32_t i = 0;
        i < 3 && particles_.size() < kMaxParticles;
        ++i
        ) {
        Particle particle{};

        const float baseScale =
            0.75f + static_cast<float>(i) * 0.32f;

        particle.transform.scale = {
            baseScale,
            baseScale,
            1.0f
        };

        particle.startScale = particle.transform.scale;

        particle.transform.rotate = {
            0.0f,
            0.0f,
            distRotate(engine)
        };

        particle.transform.translate = pos;
        particle.velocity = { 0.0f, 0.0f, 0.0f };

        if (i == 0) {
            particle.color = {
                1.0f, 1.0f, 0.75f, 1.0f
            };
        } else {
            particle.color = {
                1.0f, 0.35f,0.04f, 0.9f
            };
        }

        particle.lifeTime = 0.0f;
        particle.maxTime =
            0.12f + static_cast<float>(i) * 0.055f;

        particle.effectType = 1;

        particles_.push_back(particle);
    }

    // ゆっくり消える青い残光
    if (particles_.size() < kMaxParticles) {
        Particle particle{};

        particle.transform.scale = {
            1.35f,
            1.35f,
            1.0f
        };

        particle.startScale = particle.transform.scale;

        particle.transform.rotate = {
            0.0f,
            0.0f,
            distRotate(engine)
        };

        particle.transform.translate = pos;
        particle.velocity = { 0.0f, 0.012f, 0.0f };

        particle.color = {
            1.0f,
            0.12f,
            0.01f,
            0.5f
        };

        particle.lifeTime = 0.0f;
        particle.maxTime = 0.55f;
        particle.effectType = 2;

        particles_.push_back(particle);
    }

    // ========================================
   // ここから炎の粒を追加
   // ========================================
    std::uniform_real_distribution<float>
        flamePositionX(
            -0.35f,
            0.35f
        );

    std::uniform_real_distribution<float>
        flamePositionY(
            -0.15f,
            0.20f
        );

    std::uniform_real_distribution<float>
        flameWidth(
            0.20f,
            0.48f
        );

    std::uniform_real_distribution<float>
        flameHeight(
            0.45f,
            1.10f
        );

    std::uniform_real_distribution<float>
        flameVelocityX(
            -0.008f,
            0.008f
        );

    std::uniform_real_distribution<float>
        flameVelocityY(
            0.015f,
            0.042f
        );

    std::uniform_real_distribution<float>
        flameRotation(
            -0.28f,
            0.28f
        );

    std::uniform_real_distribution<float>
        flameLifeTime(
            0.32f,
            0.68f
        );

    const uint32_t flameCount = 24;

    for (
        uint32_t index = 0;
        index < flameCount;
        ++index
        ) {
        if (particles_.size() >=
            kMaxParticles) {
            break;
        }

        Particle particle{};

        particle.transform.scale = {
            flameWidth(engine),
            flameHeight(engine),
            1.0f
        };

        particle.startScale =
            particle.transform.scale;

        particle.transform.rotate = {
            0.0f,
            0.0f,
            flameRotation(engine)
        };

        particle.transform.translate = {
            pos.x + flamePositionX(engine),
            pos.y + flamePositionY(engine),
            pos.z
        };

        particle.velocity = {
            flameVelocityX(engine),
            flameVelocityY(engine),
            0.0f
        };

        switch (index % 3) {
        case 0:
            // 白黄色の芯
            particle.color = {
                1.0f,
                0.92f,
                0.35f,
                1.0f
            };
            break;

        case 1:
            // オレンジ色の炎
            particle.color = {
                1.0f,
                0.35f,
                0.025f,
                1.0f
            };
            break;

        default:
            // 赤い外炎
            particle.color = {
                1.0f,
                0.08f,
                0.005f,
                0.75f
            };
            break;
        }

        particle.lifeTime = 0.0f;

        particle.maxTime =
            flameLifeTime(engine);

        // HLSLの炎形状を選択
        particle.effectType = 3;

        particles_.push_back(particle);
    }
    // ========================================
    // 炎の粒ここまで
    // ========================================

} // ParticleManager::Emit()の終わり

void ParticleManager::EmitSakura(const Vector3& position, float sizeMultiplier)
{

    sizeMultiplier =
        (std::max)(sizeMultiplier, 0.01f);

    const SakuraSettings& settings =
        sakuraSettings_;

    std::uniform_real_distribution<float>
        random01(0.0f, 1.0f);

    std::uniform_real_distribution<float>
        randomAngle(
            -std::numbers::pi_v<float>,
            std::numbers::pi_v<float>
        );

    std::uniform_real_distribution<float>
        randomSize(
            settings.minSize,
            settings.maxSize
        );

    std::uniform_real_distribution<float>
        randomLifeTime(
            settings.minLifeTime,
            settings.maxLifeTime
        );

    std::uniform_real_distribution<float>
        randomRotationSpeed(
            -settings.rotationSpeed,
            settings.rotationSpeed
        );

    std::uniform_real_distribution<float>
        randomHeight(-0.35f, 0.35f);

    std::uniform_real_distribution<float>
        randomDepth(-0.25f, 0.25f);

    // 中心閃光
    if (particles_.size() < kMaxParticles) {
        Particle flash{};

        flash.transform.scale = {
            settings.flashSize * sizeMultiplier,
            settings.flashSize * sizeMultiplier,
            1.0f
        };

        flash.startScale =
            flash.transform.scale;

        flash.transform.rotate = {
            0.0f,
            0.0f,
            randomAngle(engine)
        };

        flash.transform.translate =
            position;

        flash.velocity = {
            0.0f,
            0.0f,
            0.0f
        };

        flash.acceleration = {
            0.0f,
            0.0f,
            0.0f
        };

        flash.color = {
            1.0f,
            0.82f,
            0.92f,
            1.0f
        };

        flash.angularVelocity = 0.025f;
        flash.lifeTime = 0.0f;
        flash.maxTime = settings.flashLifeTime;
        flash.effectType = 5;

        particles_.push_back(flash);
    }

    // 桜の花弁
    const int petalCount =
        std::clamp(
            settings.petalCount,
            1,
            256
        );

    for (int index = 0; index < petalCount; ++index) {
        if (particles_.size() >= kMaxParticles) {
            break;
        }

        Particle petal{};

        const float angle =
            randomAngle(engine);

        const float radius =
            std::sqrt(random01(engine)) *
            settings.spawnRadius *
            sizeMultiplier;

        const float size =
            randomSize(engine) *
            sizeMultiplier;

        const float speed =
            settings.spreadSpeed *
            (0.55f + random01(engine) * 0.9f) *
            sizeMultiplier;

        petal.transform.scale = {
            size,
            size * (1.25f + random01(engine) * 0.45f),
            1.0f
        };

        petal.startScale =
            petal.transform.scale;

        petal.transform.rotate = {
            0.0f,
            0.0f,
            randomAngle(engine)
        };

        petal.transform.translate = {
            position.x + std::cos(angle) * radius,
            position.y + randomHeight(engine) * sizeMultiplier,
            position.z + randomDepth(engine) * sizeMultiplier
        };

        petal.velocity = {
            std::cos(angle) * speed,
            settings.upwardSpeed *
                (0.35f + random01(engine)),
            std::sin(angle) * speed * 0.35f
        };

        petal.acceleration = {
            0.0f,
            -settings.gravity,
            0.0f
        };

        const float colorBlend =
            random01(engine);

        petal.color = {
            settings.color.x +
                (settings.subColor.x - settings.color.x) *
                colorBlend,

            settings.color.y +
                (settings.subColor.y - settings.color.y) *
                colorBlend,

            settings.color.z +
                (settings.subColor.z - settings.color.z) *
                colorBlend,

            1.0f
        };

        petal.angularVelocity =
            randomRotationSpeed(engine);

        petal.lifeTime = 0.0f;
        petal.maxTime = randomLifeTime(engine);
        petal.effectType = 4;

        particles_.push_back(petal);
    }

}



void ParticleManager::CreateRootSignature() {
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
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = 1;
    desc.pParameters = &rootParam;
    desc.NumStaticSamplers = 1;
    desc.pStaticSamplers = &sampler;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> blob, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &errorBlob);
    assert(SUCCEEDED(hr));
    hr = dxCommon_->GetDevice()->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));
}

void ParticleManager::CreatePipelineState() {
    // --- ★修正ポイント: InputLayoutをHLSLと完全に一致させる ---
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        // Slot 0: メッシュデータ
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        // Slot 1: インスタンスデータ (WVP行列を4つのfloat4に分割して定義)
        // HLSL側: INSTANCE_WVP0, 1, 2, 3 に対応
        { "INSTANCE_WVP", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_WVP", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_WVP", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_WVP", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },

        // Color
        { "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "EFFECT_TYPE"   , 0, DXGI_FORMAT_R32_FLOAT,1,80,D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,1},
    };

    // シェーダーコンパイル (パスにhlsl/を追加済み)
    auto vsBlob = dxCommon_->CompileShader(L"Resources/shaders/hlsl/HitParticle.VS.hlsl", L"vs_6_0");
    auto psBlob = dxCommon_->CompileShader(L"Resources/shaders/hlsl/HitParticle.PS.hlsl", L"ps_6_0");
    assert(vsBlob != nullptr && "VS Compile Failed");
    assert(psBlob != nullptr && "PS Compile Failed");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

    // ブレンド設定 (加算合成)
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

    // ★重要: ここで失敗すると pipelineState_ がNULLになり、描画時に落ちる
    HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_));
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create GraphicsPipelineState for Particles!\n");
        assert(false);
    }
}

void ParticleManager::CreateMesh() {
    VertexData vertices[] = {
        {{-0.5f,  0.5f, 0, 1}, {0.0f, 0.0f}, {0, 0, -1}},
        {{ 0.5f,  0.5f, 0, 1}, {1.0f, 0.0f}, {0, 0, -1}},
        {{-0.5f, -0.5f, 0, 1}, {0.0f, 1.0f}, {0, 0, -1}},
        {{-0.5f, -0.5f, 0, 1}, {0.0f, 1.0f}, {0, 0, -1}},
        {{ 0.5f,  0.5f, 0, 1}, {1.0f, 0.0f}, {0, 0, -1}},
        {{ 0.5f, -0.5f, 0, 1}, {1.0f, 1.0f}, {0, 0, -1}},
    };

    UINT size = sizeof(vertices);

    D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = size;
    resDesc.Height = 1; resDesc.DepthOrArraySize = 1; resDesc.MipLevels = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; resDesc.SampleDesc.Count = 1;

    HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer_));
    assert(SUCCEEDED(hr));

    VertexData* data = nullptr;
    vertexBuffer_->Map(0, nullptr, (void**)&data);
    memcpy(data, vertices, size);
    vertexBuffer_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = size;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
}