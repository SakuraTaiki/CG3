#include "Object3d.hlsli"

// =============================================
// Constant Buffers
// =============================================

ConstantBuffer<Material> gMaterial
    : register(b0);

ConstantBuffer<DirectionalLight> gDirectionalLight
    : register(b1);

ConstantBuffer<CameraForGPU> gCamera
    : register(b2);

ConstantBuffer<SpotLight> gSpotLight
    : register(b3);

ConstantBuffer<PointLight> gPointLight
    : register(b4);

// =============================================
// Textures / Sampler
// =============================================

Texture2D<float32_t4> gTexture
    : register(t0);

TextureCube<float32_t4> gEnvironmentTexture
    : register(t1);

SamplerState gSampler
    : register(s0);

// =============================================
// Pixel Shader
// =============================================

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // =========================================
    // Texture / Material
    // =========================================

    float32_t2 transformedUV =
        mul(
            float32_t4(
                input.texcoord,
                0.0f,
                1.0f
            ),
            gMaterial.uvTransform
        ).xy;

    float32_t4 textureColor =
        gTexture.Sample(
            gSampler,
            transformedUV
        );

    float32_t4 baseColor =
        textureColor *
        gMaterial.color;

    output.color = baseColor;

    // =========================================
    // Lighting
    // =========================================

    if (gMaterial.enableLighting != 0)
    {
        float32_t3 normal =
            normalize(input.normal);

        // OBJが完全な黒になることを防ぐ簡易環境光
        float32_t3 ambientColor =
            float32_t3(
                0.05f,
                0.05f,
                0.05f
            );

        // =====================================
        // DirectionalLight
        // =====================================

        float32_t3 directionalLightDirection =
            normalize(
                -gDirectionalLight.direction
            );

        float32_t directionalNdotL =
            dot(
                normal,
                directionalLightDirection
            );

        float32_t directionalDiffuse =
            pow(
                saturate(
                    directionalNdotL *
                    0.5f +
                    0.5f
                ),
                2.0f
            );

        float32_t3 directionalColor =
            gDirectionalLight.color.rgb *
            gDirectionalLight.intensity *
            directionalDiffuse;

        // =====================================
        // SpotLight
        // =====================================

        float32_t3 spotSurfaceToLight =
            gSpotLight.position -
            input.worldPosition;

        float32_t spotDistance =
            length(
                spotSurfaceToLight
            );

        float32_t3 spotSurfaceToLightDirection =
            spotSurfaceToLight /
            max(
                spotDistance,
                0.0001f
            );

        // Half-Lambert
        float32_t spotNdotL =
            dot(
                normal,
                spotSurfaceToLightDirection
            );

        float32_t spotDiffuse =
            pow(
                saturate(
                    spotNdotL *
                    0.5f +
                    0.5f
                ),
                2.0f
            );

        // 最大距離による減衰
        float32_t spotDistanceRatio =
            spotDistance /
            max(
                gSpotLight.distance,
                0.0001f
            );

        float32_t spotDistanceAttenuation =
            pow(
                saturate(
                    1.0f -
                    spotDistanceRatio
                ),
                max(
                    gSpotLight.decay,
                    0.0001f
                )
            );

        // ライト位置から物体表面へ向かう方向
        float32_t3 spotLightToSurfaceDirection =
            -spotSurfaceToLightDirection;

        float32_t3 spotDirection =
            normalize(
                gSpotLight.direction
            );

        float32_t spotCosTheta =
            dot(
                spotLightToSurfaceDirection,
                spotDirection
            );

        // 外側コーンでは0、内側コーンでは1
        float32_t spotConeAttenuation =
            smoothstep(
                gSpotLight.cosAngle,
                gSpotLight.cosFalloffStart,
                spotCosTheta
            );

        float32_t3 spotColor =
            gSpotLight.color.rgb *
            gSpotLight.intensity *
            spotDiffuse *
            spotDistanceAttenuation *
            spotConeAttenuation;

        // =====================================
        // PointLight
        // =====================================

        float32_t3 pointSurfaceToLight =
            gPointLight.position -
            input.worldPosition;

        float32_t pointDistance =
            length(
                pointSurfaceToLight
            );

        float32_t3 pointLightDirection =
            pointSurfaceToLight /
            max(
                pointDistance,
                0.0001f
            );

        // Half-Lambert
        float32_t pointNdotL =
            dot(
                normal,
                pointLightDirection
            );

        float32_t pointDiffuse =
            pow(
                saturate(
                    pointNdotL *
                    0.5f +
                    0.5f
                ),
                2.0f
            );

        // radius以上離れた場所では0
        float32_t pointDistanceRatio =
            pointDistance /
            max(
                gPointLight.radius,
                0.0001f
            );

        float32_t pointAttenuation =
            pow(
                saturate(
                    1.0f -
                    pointDistanceRatio
                ),
                max(
                    gPointLight.decay,
                    0.0001f
                )
            );

        float32_t3 pointColor =
            gPointLight.color.rgb *
            gPointLight.intensity *
            pointDiffuse *
            pointAttenuation;

        // =====================================
        // Combine Lights
        // =====================================

        float32_t3 lightingColor =
            ambientColor +
            directionalColor +
            spotColor +
            pointColor;

        output.color.rgb =
            baseColor.rgb *
            lightingColor;
    }

    // =========================================
    // Environment Mapping
    // =========================================

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

    output.color.a =
        baseColor.a;

    return output;
}