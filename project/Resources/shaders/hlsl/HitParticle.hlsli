struct HitParticleVertexOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;

    nointerpolation float effectType : TEXCOORD1;
};