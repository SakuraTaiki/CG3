struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;

    float4 parameters0 : TEXCOORD1;
    float4 parameters1 : TEXCOORD2;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(
    PixelShaderInput input
)
{
    PixelShaderOutput output;

    const float2 position =
        input.texcoord * 2.0f - 1.0f;

    const float thickness =
        saturate(input.parameters0.x * 2.0f);

    const float distortionStrength =
        max(input.parameters0.y, 0.0f);

    const float distortionFrequency =
        max(input.parameters0.z, 1.0f);

    const float distortionPhase =
        input.parameters0.w;

    const float edgeSoftness =
        max(input.parameters1.x, 0.001f);

    const float glowStrength =
        max(input.parameters1.y, 0.0f);

    const float angle =
        atan2(position.y, position.x);

    float radius =
        length(position);

    const float distortion =
        sin(
            angle * distortionFrequency +
            distortionPhase * 6.2831853f
        ) *
        distortionStrength;

    radius += distortion;

    const float innerRadius =
        saturate(1.0f - thickness);

    const float outerMask =
        1.0f -
        smoothstep(
            1.0f - edgeSoftness,
            1.0f + edgeSoftness,
            radius
        );

    const float innerMask =
        smoothstep(
            innerRadius - edgeSoftness,
            innerRadius + edgeSoftness,
            radius
        );

    const float ringAlpha =
        outerMask *
        innerMask;

    const float outerGlow =
        1.0f -
        smoothstep(
            1.0f,
            1.0f + edgeSoftness * 6.0f,
            radius
        );

    const float innerGlow =
        smoothstep(
            innerRadius - edgeSoftness * 6.0f,
            innerRadius,
            radius
        );

    const float glowAlpha =
        outerGlow *
        innerGlow *
        glowStrength;

    const float alpha =
        saturate(
            ringAlpha +
            glowAlpha
        );

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