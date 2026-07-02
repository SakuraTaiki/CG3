#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSamplerLinear : register(s0);

cbuffer RadialBlurConstants : register(b0)
{
    float2 gCenter;
    float gBlurWidth;
    float gStrength;

    uint gSampleCount;
    float3 gPadding;
};

float4 main(VertexShaderOutput input) : SV_TARGET
{
    uint sampleCount = clamp(gSampleCount, 2u, 32u);

    // 中心から現在のUVへ向かう方向
    float2 direction = input.texcoord - gCenter;

    float3 blurredColor = float3(0.0f, 0.0f, 0.0f);

    [loop]
    for (uint sampleIndex = 0;
         sampleIndex < sampleCount;
         ++sampleIndex)
    {
        float sampleOffset =
            gBlurWidth * float(sampleIndex);

        float2 sampleUv =
            input.texcoord +
            direction * sampleOffset;

        blurredColor +=
            gTexture.Sample(
                gSamplerLinear,
                sampleUv
            ).rgb;
    }

    blurredColor /= float(sampleCount);

    float4 originalColor =
        gTexture.Sample(
            gSamplerLinear,
            input.texcoord
        );

    // Strengthで元画像とブラー画像を合成
    return float4(
        lerp(
            originalColor.rgb,
            blurredColor,
            saturate(gStrength)
        ),
        originalColor.a
    );
}