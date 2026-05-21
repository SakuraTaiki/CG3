#include "Object3d.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

Texture2D<float32_t4> gTexture : register(t0);
TextureCube<float32_t4> gEnvironmentTexture : register(t1);
SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float32_t2 transformedUV =
        mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform).xy;

    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV);

    output.color = textureColor * gMaterial.color;

    if (gMaterial.enableLighting != 0)
    {
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);

        output.color.rgb *= gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
    }

    float32_t3 cameraToPosition = normalize(input.worldPosition);
    float32_t3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
    float32_t4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectedVector);

    output.color.rgb += environmentColor.rgb * gMaterial.environmentCoefficient;

    return output;
}