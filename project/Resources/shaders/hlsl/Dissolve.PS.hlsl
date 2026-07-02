#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);
Texture2D<float> gMaskTexture : register(t2);

SamplerState gSamplerLinear : register(s0);

cbuffer DissolveConstants : register(b0)
{
    float gThreshold;
    float gEdgeWidth;
    float gEdgeIntensity;
    float gPadding;

    float4 gEdgeColor;
};

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float mask =
        gMaskTexture.Sample(
            gSamplerLinear,
            input.texcoord
        );

    // Threshold以下を描画しない
    if (mask <= gThreshold)
    {
        discard;
    }

    float4 outputColor =
        gTexture.Sample(
            gSamplerLinear,
            input.texcoord
        );

    // Thresholdに近い部分をエッジにする
    float edge =
        1.0f -
        smoothstep(
            gThreshold,
            gThreshold + max(gEdgeWidth, 0.0001f),
            mask
        );

    float edgeAmount =
        saturate(
            edge * gEdgeIntensity
        );

    outputColor.rgb =
        lerp(
            outputColor.rgb,
            gEdgeColor.rgb,
            edgeAmount
        );

    return outputColor;
}