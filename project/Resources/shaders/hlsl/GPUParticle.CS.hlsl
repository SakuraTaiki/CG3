#include "GPUParticle.hlsli"

RWStructuredBuffer<GPUParticle> gParticles : register(u0);

cbuffer UpdateData : register(b2)
{
    float deltaTime;
    float totalTime;
    uint particleCount;
    float updatePad;
};

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint particleIndex = DTid.x;

    if (particleIndex >= particleCount)
    {
        return;
    }

    GPUParticle particle =
        gParticles[particleIndex];

    if (
        particle.isAlive <= 0.0f ||
        particle.maxTime <= 0.0f
    )
    {
        return;
    }

    particle.lifeTime += deltaTime;

    if (particle.lifeTime >= particle.maxTime)
    {
        particle.isAlive = 0.0f;
        particle.color.a = 0.0f;

        gParticles[particleIndex] =
            particle;

        return;
    }

    particle.velocity +=
        particle.acceleration;

    particle.translate +=
        particle.velocity;

    float time =
        saturate(
            particle.lifeTime /
            particle.maxTime
        );

    if (particle.effectType < 0.5f)
    {
        particle.scale.x =
            particle.startScale.x *
            (1.0f - time);

        particle.scale.y =
            particle.startScale.y *
            (1.0f - 0.45f * time);

        particle.color.a =
            1.0f - time;
    }
    else if (particle.effectType < 1.5f)
    {
        float scale =
            0.35f +
            1.65f * time;

        particle.scale =
            float3(
                particle.startScale.x * scale,
                particle.startScale.y * scale,
                1.0f
            );

        particle.color.a =
            (1.0f - time) *
            (1.0f - time);
    }
    else if (particle.effectType < 2.5f)
    {
        float scale =
            0.7f +
            0.8f * time;

        particle.scale =
            float3(
                particle.startScale.x * scale,
                particle.startScale.y * scale,
                1.0f
            );

        particle.rotateZ += 0.015f;

        particle.color.a =
            0.45f *
            (1.0f - time);
    }
    else if (particle.effectType < 3.5f)
    {
        float pulse =
            sin(time * 3.14159265f);

        particle.scale.x =
            particle.startScale.x *
            (0.75f + pulse * 0.45f);

        particle.scale.y =
            particle.startScale.y *
            (0.8f + time * 0.65f);

        particle.rotateZ += 0.008f;

        float fadeIn =
            min(time / 0.1f, 1.0f);

        float fadeOut =
            (1.0f - time) *
            (1.0f - time);

        particle.color.a =
            fadeIn * fadeOut;
    }
    else if (particle.effectType < 4.5f)
    {
        particle.rotateZ +=
            particle.angularVelocity;

        float flutter =
            0.72f +
            sin(
                time * 18.0f +
                particle.startScale.x * 31.0f
            ) *
            0.28f;

        particle.scale.x =
            particle.startScale.x *
            max(flutter, 0.15f);

        particle.scale.y =
            particle.startScale.y *
            (
                0.9f +
                0.15f *
                sin(time * 12.0f)
            );

        particle.translate.x +=
            sin(
                time * 13.0f +
                particle.rotateZ
            ) *
            0.004f;

        float fadeIn =
            min(time / 0.08f, 1.0f);

        float fadeOut =
            (1.0f - time) *
            (1.0f - time);

        particle.color.a =
            fadeIn * fadeOut;
    }
    else
    {
        float pulse =
            sin(time * 3.14159265f);

        float scale =
            0.25f +
            pulse * 1.35f;

        particle.scale =
            float3(
                particle.startScale.x * scale,
                particle.startScale.y * scale,
                1.0f
            );

        particle.rotateZ +=
            particle.angularVelocity;

        particle.color.a =
            (1.0f - time) *
            (1.0f - time);
    }

    gParticles[particleIndex] =
        particle;
}