#include "Skybox.hlsli"

struct Material
{
    float32_t4 color;
};

ConstantBuffer<Material> gMaterial : register(b0);
TextureCube<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

float32_t4 main(VertexShaderOutput input) : SV_TARGET0
{
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    return textureColor * gMaterial.color;
}