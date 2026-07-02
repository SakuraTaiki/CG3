#include "GPUParticle.hlsli"

RWStructuredBuffer<GPUParticle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeCounter : register(u1);

ConstantBuffer<EmitterSphere> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

uint AllocateParticleIndex()
{
    int allocatedIndex;

    InterlockedAdd(
        gFreeCounter[0],
        1,
        allocatedIndex
    );

    return
        (uint) allocatedIndex %
        kMaxParticles;
}

float RandomRange(
    inout float3 seed,
    float minValue,
    float maxValue
)
{
    return lerp(
        minValue,
        maxValue,
        Random1d(seed)
    );
}

GPUParticle CreateBaseParticle()
{
    return (GPUParticle) 0;
}

void EmitFire(inout float3 seed)
{
    uint sparkCount =
        min(gEmitter.count, kMaxParticles);

    // 放射状の火花
    for (
        uint index = 0;
        index < sparkCount;
        ++index
    )
    {
        uint particleIndex =
            AllocateParticleIndex();

        GPUParticle particle =
            CreateBaseParticle();

        float rotateZ =
            RandomRange(
                seed,
                -3.14159265f,
                3.14159265f
            );

        float speed =
            RandomRange(
                seed,
                0.035f,
                0.11f
            );

        particle.translate =
            gEmitter.translate;

        particle.scale =
            float3(
                0.035f,
                RandomRange(
                    seed,
                    0.55f,
                    1.55f
                ),
                1.0f
            ) *
            gEmitter.sizeMultiplier;

        particle.startScale =
            particle.scale;

        particle.rotateZ = rotateZ;

        particle.velocity =
            float3(
                -sin(rotateZ) *
                    speed *
                    0.55f,

                abs(cos(rotateZ)) *
                    speed +
                    0.015f,

                0.0f
            ) *
            gEmitter.sizeMultiplier;

        particle.acceleration =
            float3(
                0.0f,
                -0.0010f,
                0.0f
            );

        particle.color =
            lerp(
                gEmitter.mainColor,
                gEmitter.subColor,
                Random1d(seed)
            );

        particle.lifeTime = 0.0f;

        particle.maxTime =
            RandomRange(
                seed,
                0.22f,
                0.48f
            );

        particle.effectType = 0.0f;
        particle.isAlive = 1.0f;

        gParticles[particleIndex] =
            particle;
    }

    // 中心閃光
    for (uint index = 0; index < 3; ++index)
    {
        uint particleIndex =
            AllocateParticleIndex();

        GPUParticle particle =
            CreateBaseParticle();

        float baseScale =
            0.75f +
            (float) index * 0.32f;

        particle.translate =
            gEmitter.translate;

        particle.scale =
            float3(
                baseScale,
                baseScale,
                1.0f
            ) *
            gEmitter.sizeMultiplier;

        particle.startScale =
            particle.scale;

        particle.rotateZ =
            RandomRange(
                seed,
                -3.14159265f,
                3.14159265f
            );

        if (index == 0)
        {
            particle.color =
                float4(
                    1.0f,
                    1.0f,
                    0.75f,
                    1.0f
                );
        }
        else
        {
            particle.color =
                float4(
                    1.0f,
                    0.35f,
                    0.04f,
                    0.9f
                );
        }

        particle.maxTime =
            0.12f +
            (float) index * 0.055f;

        particle.effectType = 1.0f;
        particle.isAlive = 1.0f;

        gParticles[particleIndex] =
            particle;
    }

    // 円形残光
    {
        uint particleIndex =
            AllocateParticleIndex();

        GPUParticle particle =
            CreateBaseParticle();

        particle.translate =
            gEmitter.translate;

        particle.scale =
            float3(
                1.35f,
                1.35f,
                1.0f
            ) *
            gEmitter.sizeMultiplier;

        particle.startScale =
            particle.scale;

        particle.velocity =
            float3(
                0.0f,
                0.012f,
                0.0f
            );

        particle.color =
            float4(
                gEmitter.mainColor.rgb,
                0.5f
            );

        particle.maxTime = 0.55f;
        particle.effectType = 2.0f;
        particle.isAlive = 1.0f;

        gParticles[particleIndex] =
            particle;
    }

    // 炎
    const uint flameCount = 24;

    for (
        uint index = 0;
        index < flameCount;
        ++index
    )
    {
        uint particleIndex =
            AllocateParticleIndex();

        GPUParticle particle =
            CreateBaseParticle();

        particle.translate =
            gEmitter.translate +
            float3(
                RandomRange(
                    seed,
                    -0.35f,
                    0.35f
                ),

                RandomRange(
                    seed,
                    -0.15f,
                    0.20f
                ),

                0.0f
            ) *
            gEmitter.sizeMultiplier;

        particle.scale =
            float3(
                RandomRange(
                    seed,
                    0.20f,
                    0.48f
                ),

                RandomRange(
                    seed,
                    0.45f,
                    1.10f
                ),

                1.0f
            ) *
            gEmitter.sizeMultiplier;

        particle.startScale =
            particle.scale;

        particle.rotateZ =
            RandomRange(
                seed,
                -0.28f,
                0.28f
            );

        particle.velocity =
            float3(
                RandomRange(
                    seed,
                    -0.008f,
                    0.008f
                ),

                RandomRange(
                    seed,
                    0.015f,
                    0.042f
                ),

                0.0f
            ) *
            gEmitter.sizeMultiplier;

        if (index % 3 == 0)
        {
            particle.color =
                float4(
                    1.0f,
                    0.92f,
                    0.35f,
                    1.0f
                );
        }
        else if (index % 3 == 1)
        {
            particle.color =
                gEmitter.subColor;
        }
        else
        {
            particle.color =
                float4(
                    gEmitter.mainColor.rgb,
                    0.75f
                );
        }

        particle.maxTime =
            RandomRange(
                seed,
                0.32f,
                0.68f
            );

        particle.effectType = 3.0f;
        particle.isAlive = 1.0f;

        gParticles[particleIndex] =
            particle;
    }
}

void EmitSakura(inout float3 seed)
{
    // 中心閃光
    {
        uint particleIndex =
            AllocateParticleIndex();

        GPUParticle particle =
            CreateBaseParticle();

        particle.translate =
            gEmitter.translate;

        particle.scale =
            float3(
                1.6f,
                1.6f,
                1.0f
            ) *
            gEmitter.sizeMultiplier;

        particle.startScale =
            particle.scale;

        particle.rotateZ =
            RandomRange(
                seed,
                -3.14159265f,
                3.14159265f
            );

        particle.angularVelocity =
            0.025f;

        particle.color =
            gEmitter.subColor;

        particle.maxTime = 0.28f;
        particle.effectType = 5.0f;
        particle.isAlive = 1.0f;

        gParticles[particleIndex] =
            particle;
    }

    uint petalCount =
        min(gEmitter.count, kMaxParticles - 1);

    for (
        uint index = 0;
        index < petalCount;
        ++index
    )
    {
        uint particleIndex =
            AllocateParticleIndex();

        GPUParticle particle =
            CreateBaseParticle();

        float angle =
            RandomRange(
                seed,
                -3.14159265f,
                3.14159265f
            );

        float radius =
            sqrt(Random1d(seed)) *
            gEmitter.radius;

        float size =
            RandomRange(
                seed,
                0.08f,
                0.22f
            ) *
            gEmitter.sizeMultiplier;

        float speed =
            0.065f *
            RandomRange(
                seed,
                0.55f,
                1.45f
            ) *
            gEmitter.sizeMultiplier;

        particle.translate =
            gEmitter.translate +
            float3(
                cos(angle) * radius,

                RandomRange(
                    seed,
                    -0.35f,
                    0.35f
                ) *
                gEmitter.sizeMultiplier,

                RandomRange(
                    seed,
                    -0.25f,
                    0.25f
                ) *
                gEmitter.sizeMultiplier
            );

        particle.scale =
            float3(
                size,

                size *
                RandomRange(
                    seed,
                    1.25f,
                    1.70f
                ),

                1.0f
            );

        particle.startScale =
            particle.scale;

        particle.rotateZ =
            RandomRange(
                seed,
                -3.14159265f,
                3.14159265f
            );

        particle.velocity =
            float3(
                cos(angle) * speed,

                0.025f *
                RandomRange(
                    seed,
                    0.35f,
                    1.35f
                ),

                sin(angle) *
                speed *
                0.35f
            );

        particle.acceleration =
            float3(
                0.0f,
                -0.0012f,
                0.0f
            );

        particle.color =
            lerp(
                gEmitter.mainColor,
                gEmitter.subColor,
                Random1d(seed)
            );

        particle.angularVelocity =
            RandomRange(
                seed,
                -0.12f,
                0.12f
            );

        particle.maxTime =
            RandomRange(
                seed,
                0.55f,
                1.10f
            );

        particle.effectType = 4.0f;
        particle.isAlive = 1.0f;

        gParticles[particleIndex] =
            particle;
    }
}

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (gEmitter.emit == 0)
    {
        return;
    }

    float3 seed =
        (
            float3(DTid) +
            gEmitter.translate +
            gPerFrame.time
        ) *
        (gPerFrame.time + 1.0f);

    if (gEmitter.effectType < 0.5f)
    {
        EmitFire(seed);
    }
    else
    {
        EmitSakura(seed);
    }
}