#include "GPUParticle.hlsli"

RWStructuredBuffer<GPUParticle> gParticles : register(u0);

// ===== 変更：FreeListの末尾位置 =====
RWStructuredBuffer<int> gFreeListIndex : register(u1);

// ===== 追加：空いているParticle番号の配列 =====
RWStructuredBuffer<uint> gFreeList : register(u2);

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

    // ===== 追加：全Particle番号を空き番号として登録 =====
    gFreeList[particleIndex] = particleIndex;

    // ===== 変更：末尾は最後の有効要素を指す =====
    if (particleIndex == 0)
    {
        gFreeListIndex[0] = (int) particleCount - 1;
    }
}