struct GPUParticle
{
    float3 translate;
    float lifeTime;
    float3 scale;
    float maxTime;
    float3 startScale;
    float angularVelocity;
    float3 velocity;
    float effectType;
    float3 acceleration;
    float isAlive;
    float4 color;
    float rotateZ;
    float3 pad;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};
