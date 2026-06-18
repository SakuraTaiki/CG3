#include "HitParticle.hlsli"

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
    float effectType : EFFECT_TYPE0;
};

HitParticleVertexOutput main(VertexShaderInput input)
{
    HitParticleVertexOutput output;

    float4x4 wvp;

    wvp[0] = input.wvpRow0;
    wvp[1] = input.wvpRow1;
    wvp[2] = input.wvpRow2;
    wvp[3] = input.wvpRow3;

    output.position = mul(input.position, wvp);
    output.texcoord = input.texcoord;
    output.color = input.color;
    output.effectType = input.effectType;

    return output;
}