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

    if (input.effectType < 0.5f)
    {
        // 細長い火花
        const float horizontal = pow(
            saturate(1.0f - abs(position.x)),
            10.0f
        );

        const float vertical = pow(
            saturate(1.0f - abs(position.y)),
            1.7f
        );

        alpha = horizontal * vertical;

    }
    else if (input.effectType < 1.5f)
    {
        // 中心光と十字の光線
        const float radius = length(position);

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

        alpha = saturate(core + rayX + rayY);

    }
    else
    {
        // 青い残光
        const float radius = length(position);

        const float wobble =
            0.05f *
            sin(atan2(position.y, position.x) * 7.0f);

        alpha =
            smoothstep(
                1.0f,
                0.15f,
                radius + wobble
            ) *
            smoothstep(
                0.0f,
                0.38f,
                radius
            );
    }

    output.color = float4(
        input.color.rgb,
        input.color.a * alpha
    );

    if (output.color.a < 0.01f)
    {
        discard;
    }

    return output;
}