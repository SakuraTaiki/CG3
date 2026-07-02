#include "GPUParticle.hlsli"

RWStructuredBuffer<GPUParticle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

ConstantBuffer<EmitterSphere> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (gEmitter.emit == 0)
    {
        return;
    }

    uint emitCount = min(gEmitter.count, kMaxParticles);

    for (uint countIndex = 0; countIndex < emitCount; ++countIndex)
    {
        // FreeListの末尾を1つ減らし、減らす前の位置を受け取る。
        int freeListIndex;
        InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);

        if (freeListIndex < 0 || freeListIndex >= (int) kMaxParticles)
        {
            // 空きがない場合は減らした値を戻す。
            InterlockedAdd(gFreeListIndex[0], 1);
            break;
        }

        // FreeListに保存されている空きParticle番号を使用する。
        uint particleIndex = gFreeList[freeListIndex];

        GPUParticle particle = (GPUParticle) 0;

        // Particleごとに異なる乱数になるようにseedを作る。
        float3 randomSeed = float3(
            (float) particleIndex + gPerFrame.time * 17.0f,
            (float) countIndex + gPerFrame.time * 29.0f,
            gPerFrame.time * 43.0f + 0.123f
        );

        float3 randomValue = Random3d(randomSeed);
        float3 randomValue2 = Random3d(randomSeed);

        // [-1, 1]の範囲へ変換する。
        float3 randomDirection = randomValue * 2.0f - 1.0f;

        // 球状Emitterの範囲内に発生させる。
        float directionLength = max(length(randomDirection), 0.0001f);
        randomDirection /= directionLength;

        // powを使い、球の中へ偏りにくく分布させる。
        float spawnDistance =
            pow(saturate(randomValue2.x), 1.0f / 3.0f) *
            gEmitter.radius;

        particle.translate =
            gEmitter.translate + randomDirection * spawnDistance;

        // mainColorとsubColorの間からParticleごとの色を作る。
        particle.color = lerp(
            gEmitter.mainColor,
            gEmitter.subColor,
            randomValue2.y
        );
        particle.color.a = 1.0f;

        particle.lifeTime = 0.0f;
        particle.rotateZ = randomValue2.z * 6.28318530f;
        particle.isAlive = 1.0f;
        particle.effectType = gEmitter.effectType;

        if (gEmitter.effectType < 0.5f)
        {
            // ================================
            // 炎Particle
            // ================================
            float size =
                lerp(0.16f, 0.32f, randomValue.y) *
                gEmitter.sizeMultiplier;

            particle.scale = float3(
                size,
                size * lerp(1.1f, 1.6f, randomValue.z),
                1.0f
            );
            particle.startScale = particle.scale;

            // 炎は上へ昇り、左右へ少し広がる。
            particle.velocity = float3(
                lerp(-0.008f, 0.008f, randomValue.x),
                lerp(0.020f, 0.045f, randomValue.y),
                lerp(-0.004f, 0.004f, randomValue.z)
            );

            particle.acceleration = float3(
                0.0f,
                0.00025f,
                0.0f
            );

            particle.maxTime = lerp(0.8f, 1.8f, randomValue2.x);
            particle.angularVelocity =
                lerp(-0.035f, 0.035f, randomValue2.y);
        }
        else
        {
            // ================================
            // 桜Particle
            // ================================
            float size =
                lerp(0.10f, 0.20f, randomValue.y) *
                gEmitter.sizeMultiplier;

            particle.scale = float3(
                size,
                size * lerp(0.55f, 0.85f, randomValue.z),
                1.0f
            );
            particle.startScale = particle.scale;

            // 桜は横へ流れながらゆっくり落下する。
            particle.velocity = float3(
                lerp(-0.012f, 0.018f, randomValue.x),
                lerp(-0.012f, -0.004f, randomValue.y),
                lerp(-0.008f, 0.008f, randomValue.z)
            );

            particle.acceleration = float3(
                0.00002f,
                -0.00003f,
                0.0f
            );

            particle.maxTime = lerp(2.5f, 5.0f, randomValue2.x);
            particle.angularVelocity =
                lerp(-0.060f, 0.060f, randomValue2.y);
        }

        gParticles[particleIndex] = particle;
    }
}