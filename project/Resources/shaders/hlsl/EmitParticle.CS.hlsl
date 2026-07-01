#include "GPUParticle.hlsli"

RWStructuredBuffer<GPUParticle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeCounter : register(u1);

ConstantBuffer<EmitterSphere> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (gEmitter.emit == 0)
    {
        return;
    }

    float3 seed =
        (float3(DTid) +
            gEmitter.translate +
            gPerFrame.time) *
        (gPerFrame.time + 1.0f);

    for (
        uint countIndex = 0;
        countIndex < gEmitter.count;
        ++countIndex
    )
    {
        int particleIndex;

        InterlockedAdd(
            gFreeCounter[0],
            1,
            particleIndex
        );

        // Particleの最大数を超えた場合は生成しない。
        if (
            particleIndex < 0 ||
            particleIndex >= (int) kMaxParticles
        )
        {
            continue;
        }

        float3 random01 = Random3d(seed);
        float3 randomSigned =
            random01 * 2.0f - 1.0f;

        float angle =
            Random1d(seed) * 6.28318530718f;

        float radius =
            sqrt(Random1d(seed)) *
            gEmitter.radius;

        float sizeMultiplier =
            max(gEmitter.sizeMultiplier, 0.01f);

        GPUParticle particle =
            (GPUParticle) 0;

        particle.translate =
            gEmitter.translate +
            float3(
                cos(angle) * radius,
                randomSigned.y *
                    gEmitter.radius *
                    0.5f,
                sin(angle) *
                    radius *
                    0.35f
            );

        // 炎用Particle
        if (gEmitter.effectType < 0.5f)
        {
            float width =
                lerp(
                    0.16f,
                    0.42f,
                    Random1d(seed)
                ) *
                sizeMultiplier;

            float height =
                lerp(
                    0.38f,
                    0.95f,
                    Random1d(seed)
                ) *
                sizeMultiplier;

            particle.scale =
                float3(
                    width,
                    height,
                    1.0f
                );

            particle.velocity =
                float3(
                    cos(angle) *
                        lerp(
                            0.0075f,
                            0.0255f,
                            Random1d(seed)
                        ),

                    lerp(
                        0.015f,
                        0.055f,
                        Random1d(seed)
                    ) *
                        sizeMultiplier,

                    sin(angle) *
                        lerp(
                            0.0025f,
                            0.0085f,
                            Random1d(seed)
                        )
                );

            particle.acceleration =
                float3(
                    0.0f,
                    -0.0007f,
                    0.0f
                );

            particle.color =
                float4(
                    1.0f,

                    lerp(
                        0.12f,
                        0.55f,
                        Random1d(seed)
                    ),

                    lerp(
                        0.01f,
                        0.08f,
                        Random1d(seed)
                    ),

                    1.0f
                );

            particle.maxTime =
                lerp(
                    0.32f,
                    0.72f,
                    Random1d(seed)
                );

            particle.angularVelocity =
                lerp(
                    -0.03f,
                    0.03f,
                    Random1d(seed)
                );
        }
        // 桜用Particle
        else
        {
            float size =
                lerp(
                    0.08f,
                    0.22f,
                    Random1d(seed)
                ) *
                sizeMultiplier;

            particle.scale =
                float3(
                    size,

                    size *
                        lerp(
                            1.2f,
                            1.7f,
                            Random1d(seed)
                        ),

                    1.0f
                );

            particle.velocity =
                float3(
                    cos(angle) *
                        lerp(
                            0.025f,
                            0.07f,
                            Random1d(seed)
                        ) *
                        sizeMultiplier,

                    lerp(
                        0.006f,
                        0.03f,
                        Random1d(seed)
                    ) *
                        sizeMultiplier,

                    sin(angle) *
                        lerp(
                            0.01f,
                            0.035f,
                            Random1d(seed)
                        ) *
                        sizeMultiplier
                );

            particle.acceleration =
                float3(
                    0.0f,
                    -0.0012f,
                    0.0f
                );

            particle.color =
                float4(
                    1.0f,

                    lerp(
                        0.35f,
                        0.78f,
                        Random1d(seed)
                    ),

                    lerp(
                        0.62f,
                        0.92f,
                        Random1d(seed)
                    ),

                    1.0f
                );

            particle.maxTime =
                lerp(
                    0.7f,
                    1.35f,
                    Random1d(seed)
                );

            particle.angularVelocity =
                lerp(
                    -0.12f,
                    0.12f,
                    Random1d(seed)
                );
        }

        particle.startScale =
            particle.scale;

        particle.rotateZ =
            Random1d(seed) *
                6.28318530718f -
            3.14159265359f;

        particle.lifeTime = 0.0f;
        particle.effectType =
            gEmitter.effectType;
        particle.isAlive = 1.0f;

        gParticles[particleIndex] =
            particle;
    }
}