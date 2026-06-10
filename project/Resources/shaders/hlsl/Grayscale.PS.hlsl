#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float4 color =
        gTexture.Sample(gSampler, input.texcoord);

    float gray =
        dot(color.rgb,
            float3(0.2125f, 0.7154f, 0.0721f));

    // 純粋なグレイスケール
    return float4(gray, gray, gray, color.a);
}