#include "HitParticle.hlsli"

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(
    HitParticleVertexOutput input
)
{
    PixelShaderOutput output;

    const float2 position =
        input.texcoord * 2.0f - 1.0f;

    float alpha = 0.0f;
    float3 outputRgb =
        input.color.rgb;

    if (input.effectType < 0.5f)
    {
        // 火花
        const float horizontal = pow(
            saturate(1.0f - abs(position.x)),
            10.0f
        );

        const float vertical = pow(
            saturate(1.0f - abs(position.y)),
            1.7f
        );

        alpha =
            horizontal *
            vertical;
    }
    else if (input.effectType < 1.5f)
    {
        // 中心閃光
        const float radius =
            length(position);

        const float core = pow(
            saturate(1.0f - radius),
            3.0f
        );

        const float rayX =
            pow(
                saturate(
                    1.0f - abs(position.y) * 9.0f
                ),
                3.0f
            ) *
            saturate(1.0f - abs(position.x));

        const float rayY =
            pow(
                saturate(
                    1.0f - abs(position.x) * 9.0f
                ),
                3.0f
            ) *
            saturate(1.0f - abs(position.y));

        alpha =
            saturate(core + rayX + rayY);
    }
    else if (input.effectType < 2.5f)
    {
        // 円形残光
        const float radius =
            length(position);

        const float wobble =
            0.05f *
            sin(
                atan2(position.y, position.x) *
                7.0f
            );

        const float outerFade =
            1.0f -
            smoothstep(
                0.7f,
                1.0f,
                radius + wobble
            );

        const float innerFade =
            smoothstep(
                0.15f,
                0.38f,
                radius
            );

        alpha =
            outerFade *
            innerFade;
    }
    else if (input.effectType < 3.5f)
    {
        // 炎
        const float height =
            1.0f - input.texcoord.y;

        float flameX =
            position.x;

        const float bend =
            sin(
                height * 9.0f +
                input.color.g * 5.0f
            ) *
            0.13f *
            height;

        flameX += bend;

        const float flameWidth =
            lerp(
                0.62f,
                0.06f,
                pow(height, 0.72f)
            );

        const float sideFade =
            1.0f -
            smoothstep(
                flameWidth * 0.65f,
                flameWidth,
                abs(flameX)
            );

        const float bottomFade =
            smoothstep(
                0.0f,
                0.10f,
                height
            );

        const float topFade =
            1.0f -
            smoothstep(
                0.72f,
                1.0f,
                height
            );

        const float innerNoise =
            0.82f +
            0.18f *
            sin(
                height * 18.0f +
                flameX * 12.0f
            );

        alpha =
            sideFade *
            bottomFade *
            topFade *
            innerNoise;

        const float heat =
            pow(
                1.0f - height,
                2.0f
            );

        const float3 hotColor =
            float3(
                1.0f,
                1.0f,
                0.55f
            );

        outputRgb =
            lerp(
                input.color.rgb,
                hotColor,
                heat * 0.75f
            );
    }
    else if (input.effectType < 4.5f)
    {
        // =====================================
        // 桜の花弁
        // =====================================
        float2 petalPosition =
            position;

        petalPosition.y += 0.05f;

        // 左右2枚の丸みを合成して花弁形状を作る
        const float leftDistance =
            length(
                (petalPosition -
                    float2(-0.23f, -0.08f)) *
                float2(1.20f, 0.82f)
            );

        const float rightDistance =
            length(
                (petalPosition -
                    float2(0.23f, -0.08f)) *
                float2(1.20f, 0.82f)
            );

        const float leftLobe =
            1.0f -
            smoothstep(
                0.67f,
                0.78f,
                leftDistance
            );

        const float rightLobe =
            1.0f -
            smoothstep(
                0.67f,
                0.78f,
                rightDistance
            );

        float petalShape =
            max(leftLobe, rightLobe);

        // 下側を細く尖らせる
        const float bottomWidth =
            saturate(
                (0.96f - petalPosition.y) *
                1.4f
            );

        const float bottomMask =
            1.0f -
            smoothstep(
                bottomWidth * 0.55f,
                bottomWidth * 0.70f + 0.02f,
                abs(petalPosition.x)
            );

        petalShape *=
            lerp(
                bottomMask,
                1.0f,
                saturate(
                    1.0f -
                    (petalPosition.y + 0.15f)
                )
            );

        // 花弁上部の切れ込み
        const float notch =
            1.0f -
            smoothstep(
                0.08f,
                0.24f,
                length(
                    petalPosition -
                    float2(0.0f, -0.72f)
                )
            );

        petalShape *=
            1.0f - notch * 0.85f;

        // 中央に薄い筋を入れる
        const float centerLine =
            pow(
                saturate(
                    1.0f -
                    abs(petalPosition.x) * 7.0f
                ),
                3.0f
            ) *
            saturate(
                1.0f -
                abs(petalPosition.y)
            );

        outputRgb =
            lerp(
                input.color.rgb,
                float3(1.0f, 0.90f, 0.96f),
                centerLine * 0.45f
            );

        alpha =
            saturate(
                petalShape +
                centerLine * 0.12f
            );
    }
    else
    {
        // =====================================
        // 桜の中心閃光
        // =====================================
        const float radius =
            length(position);

        const float center =
            pow(
                saturate(1.0f - radius),
                4.0f
            );

        const float horizontal =
            pow(
                saturate(
                    1.0f -
                    abs(position.y) * 13.0f
                ),
                2.5f
            ) *
            saturate(1.0f - abs(position.x));

        const float diagonalA =
            pow(
                saturate(
                    1.0f -
                    abs(position.x + position.y) *
                    7.0f
                ),
                3.0f
            );

        const float diagonalB =
            pow(
                saturate(
                    1.0f -
                    abs(position.x - position.y) *
                    7.0f
                ),
                3.0f
            );

        alpha =
            saturate(
                center +
                horizontal +
                diagonalA * 0.45f +
                diagonalB * 0.45f
            );

        outputRgb =
            lerp(
                input.color.rgb,
                float3(1.0f, 1.0f, 1.0f),
                center
            );
    }

    output.color = float4(
        outputRgb,
        input.color.a * alpha
    );

    if (output.color.a < 0.01f)
    {
        discard;
    }

    return output;
}