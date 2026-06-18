struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;

    float4 effectParameters : TEXCOORD1;
    float4 noiseParameters : TEXCOORD2;

    float heightRatio : TEXCOORD3;
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

    const float topFade =
        max(
            input.effectParameters.y,
            0.001f
        );

    const float bottomFade =
        max(
            input.effectParameters.z,
            0.001f
        );

    // texcoord.yは上が0、下が1
    const float topMask =
        smoothstep(
            0.0f,
            topFade,
            input.texcoord.y
        );

    const float bottomMask =
        1.0f -
        smoothstep(
            1.0f - bottomFade,
            1.0f,
            input.texcoord.y
        );

    const float noiseStrength =
        saturate(
            input.noiseParameters.x
        );

    const float noiseFrequency =
        max(
            input.noiseParameters.y,
            1.0f
        );

    const float noisePhase =
        input.noiseParameters.z;

    const float waveA =
        sin(
            input.texcoord.x *
            noiseFrequency *
            6.2831853f +
            noisePhase * 4.0f
        );

    const float waveB =
        sin(
            input.texcoord.y *
            noiseFrequency *
            9.0f -
            noisePhase * 2.7f
        );

    const float noise =
        0.5f +
        0.25f * waveA +
        0.25f * waveB;

    const float noiseMask =
        lerp(
            1.0f,
            saturate(noise),
            noiseStrength
        );

    // 円周方向へ明暗を付ける
    const float circumferenceGlow =
        0.65f +
        0.35f *
        pow(
            abs(
                sin(
                    input.texcoord.x *
                    3.14159265f
                )
            ),
            0.6f
        );

    const float alpha =
        input.color.a *
        topMask *
        bottomMask *
        noiseMask;

    const float3 finalColor =
        input.color.rgb *
        circumferenceGlow;

    output.color = float4(
        finalColor,
        alpha
    );

    if (output.color.a < 0.01f)
    {
        discard;
    }

    return output;
}