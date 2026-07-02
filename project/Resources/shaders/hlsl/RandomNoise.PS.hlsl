#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSamplerLinear : register(s0);

cbuffer RandomConstants : register(b0)
{
    float gTime;
    float gScale;
    float gStrength;
    float gSpeed;

    uint gShowNoiseOnly;
    float3 gPadding;
};

// 0.0以上1.0未満の疑似乱数を生成
float rand2dTo1d(float2 value)
{
    float2 smallValue =
        sin(value);

    float random =
        dot(
            smallValue,
            float2(
                12.9898f,
                78.233f
            )
        );

    random =
        frac(
            sin(random) *
            143758.5453f
        );

    return random;
}

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float scale =
        max(gScale, 1.0f);

    // 同じブロック内では同じ乱数にする
    float2 pixelSeed =
        floor(
            input.texcoord * scale
        );

    // 時間をSeedへ加えて変化させる
    float timeSeed =
        floor(
            gTime * gSpeed * 60.0f
        );

    float random =
        rand2dTo1d(
            pixelSeed +
            float2(
                timeSeed,
                timeSeed * 0.73f
            )
        );

    if (gShowNoiseOnly != 0)
    {
        return float4(
            random,
            random,
            random,
            1.0f
        );
    }

    float4 originalColor =
        gTexture.Sample(
            gSamplerLinear,
            input.texcoord
        );

    // Strengthが0なら元画像、1なら乱数を乗算
    float noiseMultiplier =
        lerp(
            1.0f,
            random,
            saturate(gStrength)
        );

    originalColor.rgb *=
        noiseMultiplier;

    return originalColor;
}