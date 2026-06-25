#include "GPUParticle.hlsli"

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

StructuredBuffer<GPUParticle> gParticles : register(t1);

cbuffer ViewProjectionData : register(b0)
{
    float4x4 billboard;
    float4x4 viewProjection;
};

float4x4 MakeScale(float3 scale)
{
    return float4x4(
        scale.x, 0.0f, 0.0f, 0.0f,
        0.0f, scale.y, 0.0f, 0.0f,
        0.0f, 0.0f, scale.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

float4x4 MakeRotateZ(float radian)
{
    float s = sin(radian);
    float c = cos(radian);
    return float4x4(
        c, s, 0.0f, 0.0f,
        -s, c, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

float4x4 MakeTranslate(float3 translate)
{
    return float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        translate.x, translate.y, translate.z, 1.0f
    );
}

VertexShaderOutput main(
    VertexShaderInput input,
    uint instanceId : SV_InstanceID
)
{
    VertexShaderOutput output;

    GPUParticle particle = gParticles[instanceId];
    if (particle.isAlive <= 0.0f || particle.color.a <= 0.0f)
    {
        output.position = float4(0.0f, 0.0f, 0.0f, 0.0f);
        output.texcoord = input.texcoord;
        output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return output;
    }

    float4x4 world =
        mul(
            mul(MakeScale(particle.scale), MakeRotateZ(particle.rotateZ)),
            mul(billboard, MakeTranslate(particle.translate))
        );

    output.position =
        mul(mul(input.position, world), viewProjection);
    output.texcoord = input.texcoord;
    output.color = particle.color;

    return output;
}
