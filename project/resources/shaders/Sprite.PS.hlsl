#include "Sprite.hlsli"

// マテリアル
cbuffer Material : register(b0)
{
    float32_t4 color;
    float32_t4x4 uvTransform;
};

// テクスチャ
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// 出力
struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    // ★ここ修正
    output.color = color * textureColor;

    return output;
}