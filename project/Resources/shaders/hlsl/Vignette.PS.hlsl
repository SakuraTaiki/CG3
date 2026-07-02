#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer PostEffectConstants : register(b0)
{
    float gVignetteIntensity;
    float gVignetteRadius;
    float gVignetteSoftness;
    float gAspectRatio;

    uint gEnableVignette;
    uint gEnableGrayScale;
    uint2 gPadding;
};

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float4 color =
        gTexture.Sample(gSampler, input.texcoord);

    if (gEnableGrayScale != 0)
    {
        float gray = dot(
            color.rgb,
            float3(0.2125f, 0.7154f, 0.0721f)
        );

        color.rgb = gray.xxx;
    }

    if (gEnableVignette != 0)
    {
        float2 centeredUV =
            input.texcoord - 0.5f;

        // ウィンドウの縦横比を補正
        centeredUV.x *= gAspectRatio;

        float distanceFromCenter =
            length(centeredUV);

        float edge = smoothstep(
            gVignetteRadius,
            gVignetteRadius +
                max(gVignetteSoftness, 0.0001f),
            distanceFromCenter
        );

        color.rgb *=
            1.0f -
            saturate(edge * gVignetteIntensity);
    }

    return color;
}