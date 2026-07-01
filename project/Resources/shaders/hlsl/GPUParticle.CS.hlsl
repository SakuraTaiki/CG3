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

    GPUParticle particle = gParticles[particleIndex];
    if (particle.isAlive <= 0.0f || particle.maxTime <= 0.0f)
    {
        return;
    }

    particle.lifeTime += deltaTime;
    if (particle.lifeTime >= particle.maxTime)
    {
        particle.isAlive = 0.0f;
        particle.color.a = 0.0f;
        gParticles[particleIndex] = particle;
        return;
    }

    particle.velocity += particle.acceleration;
    particle.translate += particle.velocity;
    particle.rotateZ += particle.angularVelocity;

    float time = saturate(particle.lifeTime / particle.maxTime);

    if (particle.effectType < 0.5f)
    {
        float pulse = sin(time * 3.14159265f);
        particle.scale.x = particle.startScale.x * (0.75f + pulse * 0.45f);
        particle.scale.y = particle.startScale.y * (0.8f + time * 0.65f);
        particle.color.a = (1.0f - time) * (1.0f - time);
    }
    else
    {
        float flutter = 0.72f + sin(time * 18.0f + particle.startScale.x * 31.0f) * 0.28f;
        particle.scale.x = particle.startScale.x * max(flutter, 0.15f);
        particle.scale.y = particle.startScale.y * (0.9f + 0.15f * sin(time * 12.0f));
        particle.translate.x += sin(time * 13.0f + particle.rotateZ) * 0.004f;
        float fadeIn = min(time / 0.08f, 1.0f);
        float fadeOut = (1.0f - time) * (1.0f - time);
        particle.color.a = fadeIn * fadeOut;
    }

    gParticles[particleIndex] = particle;
}
