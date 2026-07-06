#include "Object3d.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<CameraForGPU> gCamera:register(b2);
ConstantBuffer<SpotLight> gSpotLight : register(b3);

Texture2D<float32_t4> gTexture : register(t0);
TextureCube<float32_t4> gEnvironmentTexture : register(t1);
SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float32_t2 transformedUV =
        mul(
            float32_t4(input.texcoord, 0.0f, 1.0f),
            gMaterial.uvTransform
        ).xy;

    float32_t4 textureColor =
        gTexture.Sample(gSampler, transformedUV);

    float32_t4 baseColor =
        textureColor * gMaterial.color;

    output.color = baseColor;

    if (gMaterial.enableLighting != 0)
    {
        float32_t3 normal = normalize(input.normal);

        // DirectionalLight
        float32_t directionalNdotL =
            dot(normal, -gDirectionalLight.direction);

        float32_t directionalDiffuse =
            pow(
                saturate(
                    directionalNdotL * 0.5f + 0.5f
                ),
                2.0f
            );

        float32_t3 directionalColor =
            gDirectionalLight.color.rgb *
            gDirectionalLight.intensity *
            directionalDiffuse;

        // SpotLightまでのベクトル
        float32_t3 surfaceToLight =
            gSpotLight.position - input.worldPosition;

        float32_t distanceToLight =
            length(surfaceToLight);

        float32_t3 lightDirection =
            surfaceToLight /
            max(distanceToLight, 0.0001f);

        // Half-Lambert拡散反射
        float32_t spotNdotL =
            dot(normal, lightDirection);

        float32_t spotDiffuse =
            pow(
                saturate(spotNdotL * 0.5f + 0.5f),
                2.0f
            );

        // 距離減衰
        float32_t distanceFactor =
            pow(
                saturate(
                    1.0f -
                    distanceToLight /
                    max(gSpotLight.distance, 0.0001f)
                ),
                gSpotLight.decay
            );

        // ライトから物体へ向かう方向
        float32_t3 lightToSurface =
            -lightDirection;

        float32_t cosTheta =
            dot(
                lightToSurface,
                normalize(gSpotLight.direction)
            );

        // 内側では1、外側では0
        float32_t coneFactor =
            smoothstep(
                gSpotLight.cosAngle,
                gSpotLight.cosFalloffStart,
                cosTheta
            );

        float32_t3 spotColor =
            gSpotLight.color.rgb *
            gSpotLight.intensity *
            spotDiffuse *
            distanceFactor *
            coneFactor;

        output.color.rgb =
            baseColor.rgb *
            (directionalColor + spotColor);
    }

    float32_t3 cameraToPosition =
        normalize(
            input.worldPosition -
            gCamera.worldPosition
        );

    float32_t3 reflectedVector =
        reflect(
            cameraToPosition,
            normalize(input.normal)
        );

    float32_t4 environmentColor =
        gEnvironmentTexture.Sample(
            gSampler,
            reflectedVector
        );

    output.color.rgb +=
        environmentColor.rgb *
        gMaterial.environmentCoefficient;

    output.color.a = baseColor.a;

    return output;
}