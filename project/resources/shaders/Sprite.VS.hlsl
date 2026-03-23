#include "Sprite.hlsli"

// 定数バッファ
cbuffer TransformationMatrix : register(b0)
{
    float32_t4x4 WVP;
};

// 入力
struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
};

// メイン
VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    // ★ここ修正
    output.position = mul(input.position, WVP);

    output.texcoord = input.texcoord;

    return output;
}