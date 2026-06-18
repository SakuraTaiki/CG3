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

    float4 shapeParameters : INSTANCE_SHAPE0;
    float4 effectParameters : INSTANCE_EFFECT0;
    float4 noiseParameters : INSTANCE_NOISE0;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;

    float4 effectParameters : TEXCOORD1;
    float4 noiseParameters : TEXCOORD2;

    float heightRatio : TEXCOORD3;
};

VertexShaderOutput main(
    VertexShaderInput input
)
{
    VertexShaderOutput output;

    float4 localPosition =
        input.position;

    const float heightRatio =
        saturate(localPosition.y);

    const float radiusScale =
        lerp(
            input.shapeParameters.x,
            input.shapeParameters.y,
            heightRatio
        );

    localPosition.xz *=
        radiusScale;

    const float twist =
        (
            input.shapeParameters.z +
            input.shapeParameters.w
        ) *
        heightRatio;

    const float sine =
        sin(twist);

    const float cosine =
        cos(twist);

    const float2 rotatedXZ =
        float2(
            localPosition.x * cosine -
            localPosition.z * sine,

            localPosition.x * sine +
            localPosition.z * cosine
        );

    localPosition.x =
        rotatedXZ.x;

    localPosition.z =
        rotatedXZ.y;

    float4x4 wvp;

    wvp[0] = input.wvpRow0;
    wvp[1] = input.wvpRow1;
    wvp[2] = input.wvpRow2;
    wvp[3] = input.wvpRow3;

    output.position =
        mul(localPosition, wvp);

    output.texcoord =
        input.texcoord;

    output.color =
        input.color;

    output.effectParameters =
        input.effectParameters;

    output.noiseParameters =
        input.noiseParameters;

    output.heightRatio =
        heightRatio;

    return output;
}