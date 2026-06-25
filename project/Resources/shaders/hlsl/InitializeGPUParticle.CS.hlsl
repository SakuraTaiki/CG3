#include "GPUParticle.hlsli"

RWStructuredBuffer<GPUParticle> gParticles : register(u0);

cbuffer UpdateData : register(b0)
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

    gParticles[particleIndex] = (GPUParticle) 0;

    // Visual check from the lesson:
    // gParticles[particleIndex].scale = float3(0.5f, 0.5f, 0.5f);
    // gParticles[particleIndex].color = float4(1.0f, 1.0f, 1.0f, 1.0f);
}