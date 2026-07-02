#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);

SamplerState gSamplerLinear : register(s0);
SamplerState gSamplerPoint : register(s1);

cbuffer OutlineConstants : register(b0)
{
    float4 gOutlineColor;

    float gOutlineThreshold;
    float gOutlineStrength;
    float gNearClip;
    float gFarClip;

    uint gOutlineThickness;
    uint gEnableOutline;
    float2 gPadding;
};

static const float kPrewittHorizontal[3][3] =
{
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f }
};

static const float kPrewittVertical[3][3] =
{
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f }
};

// DirectXのNDC深度をView空間の距離に戻す
float ConvertToViewZ(float ndcDepth)
{
    return
        (gNearClip * gFarClip) /
        max(
            gFarClip -
            ndcDepth * (gFarClip - gNearClip),
            0.00001f
        );
}

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float4 originalColor =
        gTexture.Sample(
            gSamplerLinear,
            input.texcoord
        );

    if (gEnableOutline == 0)
    {
        return originalColor;
    }

    uint width;
    uint height;

    gDepthTexture.GetDimensions(
        width,
        height
    );

    float2 texelSize =
        float2(
            rcp((float) width),
            rcp((float) height)
        );

    float2 difference = float2(0.0f, 0.0f);

    int thickness =
        clamp(
            (int) gOutlineThickness,
            1,
            4
        );

    [unroll]
    for (int y = 0; y < 3; ++y)
    {
        [unroll]
        for (int x = 0; x < 3; ++x)
        {
            int2 kernelPosition =
                int2(x - 1, y - 1);

            float2 sampleUV =
                input.texcoord +
                float2(kernelPosition) *
                texelSize *
                (float) thickness;

            float ndcDepth =
                gDepthTexture.Sample(
                    gSamplerPoint,
                    sampleUV
                );

            float viewZ =
                ConvertToViewZ(ndcDepth);

            difference.x +=
                viewZ *
                kPrewittHorizontal[y][x];

            difference.y +=
                viewZ *
                kPrewittVertical[y][x];
        }
    }

    float edgeStrength =
        length(difference);

    float outline =
        smoothstep(
            gOutlineThreshold,
            gOutlineThreshold * 2.0f,
            edgeStrength
        );

    outline =
        saturate(
            outline *
            gOutlineStrength
        );

    originalColor.rgb =
        lerp(
            originalColor.rgb,
            gOutlineColor.rgb,
            outline * gOutlineColor.a
        );

    return originalColor;
}