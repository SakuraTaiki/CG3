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
        // 細長い火花
        const float horizontal = pow(
            saturate(
                1.0f - abs(position.x)
            ),
            10.0f
        );

        const float vertical = pow(
            saturate(
                1.0f - abs(position.y)
            ),
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
            saturate(
                1.0f - radius
            ),
            3.0f
        );

        const float rayX =
            pow(
                saturate(
                    1.0f -
                    abs(position.y) * 9.0f
                ),
                3.0f
            ) *
            saturate(
                1.0f - abs(position.x)
            );

        const float rayY =
            pow(
                saturate(
                    1.0f -
                    abs(position.x) * 9.0f
                ),
                3.0f
            ) *
            saturate(
                1.0f - abs(position.y)
            );

        alpha =
            saturate(
                core +
                rayX +
                rayY
            );
    }
    else if (input.effectType < 2.5f)
    {
        // 円形の残光
        const float radius =
            length(position);

        const float wobble =
            0.05f *
            sin(
                atan2(
                    position.y,
                    position.x
                ) * 7.0f
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
    else
    {
        // =================================
        // 炎粒子
        // =================================

        // 下端を0、上端を1にする
        const float height =
            1.0f -
            input.texcoord.y;

        float flameX =
            position.x;

        // 上へ行くほど大きく曲げる
        const float bend =
            sin(
                height * 9.0f +
                input.color.g * 5.0f
            ) *
            0.13f *
            height;

        flameX += bend;

        // 下は太く、上は細くする
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

        // 炎の下端を滑らかにする
        const float bottomFade =
            smoothstep(
                0.0f,
                0.10f,
                height
            );

        // 炎の先端を滑らかに消す
        const float topFade =
            1.0f -
            smoothstep(
                0.72f,
                1.0f,
                height
            );

        // 内側に小さな揺らぎを作る
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

        // 下側を白黄色にする
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