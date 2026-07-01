#include "GPUParticle.hlsli"

RWStructuredBuffer<GPUParticle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeCounter : register(u1);

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

    gParticles[particleIndex] = (GPUParticle) 0;

    // 全threadが同じCounterを書かないよう、0番だけが初期化する。
    if (particleIndex == 0)
    {
        gFreeCounter[0] = 0;
    }
}