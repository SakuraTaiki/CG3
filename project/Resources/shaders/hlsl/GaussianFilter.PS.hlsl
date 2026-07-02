#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer GaussianConstants : register(b0)
{
    // 1=3x3、2=5x5、3=7x7、4=9x9
    uint gBlurRadius;

    // 標準偏差。大きいほど広くぼける
    float gSigma;

    // 0.0=元画像、1.0=Gaussian適用画像
    float gBlurStrength;

    float gPadding;
};

float GaussianWeight(
    float x,
    float y,
    float sigma
)
{
    float sigmaSquared =
        sigma * sigma;

    float exponent =
        -(x * x + y * y) /
        (2.0f * sigmaSquared);

    return exp(exponent);
}

float4 main(
    VertexShaderOutput input
) : SV_TARGET
{
    uint width;
    uint height;

    gTexture.GetDimensions(
        width,
        height
    );

    float2 texelSize = float2(
        rcp((float) width),
        rcp((float) height)
    );

    int radius = clamp(
        (int) gBlurRadius,
        1,
        4
    );

    float sigma = max(
        gSigma,
        0.01f
    );

    float4 gaussianColor =
        float4(
            0.0f,
            0.0f,
            0.0f,
            0.0f
        );

    float totalWeight = 0.0f;

    [loop]
    for (
        int y = -radius;
        y <= radius;
        ++y
    )
    {
        [loop]
        for (
            int x = -radius;
            x <= radius;
            ++x
        )
        {
            float weight =
                GaussianWeight(
                    (float) x,
                    (float) y,
                    sigma
                );

            float2 offset =
                float2(
                    (float) x,
                    (float) y
                ) * texelSize;

            gaussianColor +=
                gTexture.Sample(
                    gSampler,
                    input.texcoord + offset
                ) * weight;

            totalWeight += weight;
        }
    }

    gaussianColor /=
        max(totalWeight, 0.0001f);

    float4 originalColor =
        gTexture.Sample(
            gSampler,
            input.texcoord
        );

    return lerp(
        originalColor,
        gaussianColor,
        saturate(gBlurStrength)
    );
}