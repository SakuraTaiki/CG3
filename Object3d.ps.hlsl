#include"Object3d.hlsli"


ConstantBuffer<Material> gMaterial : register(b0);

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

Texture2D<float32_t4> gTexture : register(t0);

SamplerState gSampler : register(s0);


struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};





PixelShaderOutput main(VertexShaderOutput input)
{
    float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler,transformedUV.xy);
    
    PixelShaderOutput output;
    
    output.color = gMaterial.color * textureColor;
    
    
    if (textureColor.a <= 0.5)
    {
        discard;
    }
    
    if (textureColor.a == 0.0)
    {
        discard;
    }
    
    if (output.color.a == 0.0)
    {
        discard;
    }
    
    
    if (gMaterial.enableLighting != 0)
    {
        //float cos = saturate(dot(normalize(input.normal), -gDirectionalLight.direction));

        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        output.color = gMaterial.color * textureColor * gDirectionalLight.color * cos * gDirectionalLight.intencity;
        
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb *
        gDirectionalLight.color.rgb * cos * gDirectionalLight.intencity;
      
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    
    
    
    return output;
}