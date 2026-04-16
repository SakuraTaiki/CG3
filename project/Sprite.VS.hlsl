// 定数バッファ（VS : b0）
cbuffer TransformationMatrix : register(b0)
{
    float4x4 WVP;
};

// 入力
struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};

// 出力（PSへ渡す）
struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// メイン
VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    output.position = mul(input.position, WVP);
    output.texcoord = input.texcoord;

    return output;
}