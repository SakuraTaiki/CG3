// VSから受け取る
struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// マテリアル（PS : b0）
cbuffer Material : register(b0)
{
    float4 color;
    float4x4 uvTransform; // 今は未使用（将来用）
};

// テクスチャ
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// 出力
struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    output.color = color * textureColor;

    return output;
}