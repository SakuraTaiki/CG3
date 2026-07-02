#include "GPUParticle.hlsli"

RWStructuredBuffer<GPUParticle> gParticles : register(u0);

// ===== 追加：寿命切れParticleをFreeListへ戻すために使用 =====
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

cbuffer UpdateData : register(b2)
{
    float deltaTime;
    float totalTime;
    uint particleCount;
    float updatePad;
};

// Particleを描画対象外にし、番号をFreeListへ戻す。
void FreeParticle(uint particleIndex, inout GPUParticle particle)
{
    particle.isAlive = 0.0f;
    particle.color.a = 0.0f;
    particle.scale = float3(0.0f, 0.0f, 0.0f);

    // Particleを無効化してから、その番号をFreeListへ戻す。
    gParticles[particleIndex] = particle;

    int oldFreeListIndex;
    InterlockedAdd(gFreeListIndex[0], 1, oldFreeListIndex);

    int newFreeListIndex = oldFreeListIndex + 1;

    if (newFreeListIndex < (int) kMaxParticles)
    {
        gFreeList[newFreeListIndex] = particleIndex;
    }
    else
    {
        // 通常は到達しない。範囲外書き込みを防ぐため値を戻す。
        InterlockedAdd(gFreeListIndex[0], -1);
    }
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint particleIndex = DTid.x;

    if (particleIndex >= particleCount)
    {
        return;
    }

    GPUParticle particle = gParticles[particleIndex];

    if (particle.isAlive <= 0.0f)
    {
        return;
    }

    if (particle.maxTime <= 0.0f)
    {
        // ===== 変更：無効化するだけでなくFreeListへ戻す =====
        FreeParticle(particleIndex, particle);
        return;
    }

    particle.lifeTime += deltaTime;

    if (particle.lifeTime >= particle.maxTime)
    {
        particle.lifeTime = particle.maxTime;

        // ===== 変更：寿命が尽きた番号をFreeListへ戻す =====
        FreeParticle(particleIndex, particle);
        return;
    }

    particle.velocity += particle.acceleration;
    particle.translate += particle.velocity;
    particle.rotateZ += particle.angularVelocity;

    float normalizedTime =
        saturate(particle.lifeTime / particle.maxTime);

    if (particle.effectType < 0.5f)
    {
        float pulse = sin(normalizedTime * 3.14159265f);
        particle.scale.x =
            particle.startScale.x * (0.75f + pulse * 0.45f);
        particle.scale.y =
            particle.startScale.y * (0.8f + normalizedTime * 0.65f);
        particle.color.a =
            (1.0f - normalizedTime) * (1.0f - normalizedTime);
    }
    else
    {
        float flutter =
            0.72f +
            sin(normalizedTime * 18.0f + particle.startScale.x * 31.0f) *
            0.28f;

        particle.scale.x =
            particle.startScale.x * max(flutter, 0.15f);
        particle.scale.y =
            particle.startScale.y *
            (0.9f + 0.15f * sin(normalizedTime * 12.0f));
        particle.translate.x +=
            sin(normalizedTime * 13.0f + particle.rotateZ) * 0.004f;

        float fadeIn = min(normalizedTime / 0.08f, 1.0f);
        float fadeOut =
            (1.0f - normalizedTime) * (1.0f - normalizedTime);
        particle.color.a = fadeIn * fadeOut;
    }

    gParticles[particleIndex] = particle;
}