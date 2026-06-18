struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;

    float4 wvpRow0 : INSTANCE_WVP0;
    float4 wvpRow1 : INSTANCE_WVP1;
    float4 wvpRow2 : INSTANCE_WVP2;
    float4 wvpRow3 : INSTANCE_WVP3;

    float4 color : INSTANCE_COLOR;

    float4 parameters0 : INSTANCE_PARAMETERS0;
    float4 parameters1 : INSTANCE_PARAMETERS1;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;

    float4 parameters0 : TEXCOORD1;
    float4 parameters1 : TEXCOORD2;
};

VertexShaderOutput main(
    VertexShaderInput input
)
{
    VertexShaderOutput output;

    float4x4 wvp;

    wvp[0] = input.wvpRow0;
    wvp[1] = input.wvpRow1;
    wvp[2] = input.wvpRow2;
    wvp[3] = input.wvpRow3;

    output.position =
        mul(input.position, wvp);

    output.texcoord =
        input.texcoord;

    output.color =
        input.color;

    output.parameters0 =
        input.parameters0;

    output.parameters1 =
        input.parameters1;

    return output;
}