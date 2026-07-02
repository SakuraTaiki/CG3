#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer SmoothingConstants : register(b0)
{
    // 1=3x3、2=5x5、3=7x7、4=9x9
    uint gBlurRadius;

    // 0.0=元画像、1.0=完全なぼかし
    float gBlurStrength;

    float2 gPadding;
};

float4 main(VertexShaderOutput input) : SV_TARGET
{
    uint width;
    uint height;

    gTexture.GetDimensions(
        width,
        height
    );

    float2 texelSize = float2(
        rcp((float) width),
        rcp((float) height)
    );

    int radius = clamp(
        (int) gBlurRadius,
        1,
        4
    );

    float4 blurColor =
        float4(
            0.0f,
            0.0f,
            0.0f,
            0.0f
        );

    int sampleCount = 0;

    [loop]
    for (int y = -radius; y <= radius; ++y)
    {
        [loop]
        for (int x = -radius; x <= radius; ++x)
        {
            float2 offset =
                float2(
                    (float) x,
                    (float) y
                ) * texelSize;

            blurColor +=
                gTexture.Sample(
                    gSampler,
                    input.texcoord + offset
                );

            ++sampleCount;
        }
    }

    blurColor /= (float) sampleCount;

    float4 originalColor =
        gTexture.Sample(
            gSampler,
            input.texcoord
        );

    float4 result = lerp(
        originalColor,
        blurColor,
        saturate(gBlurStrength)
    );

    return result;
}