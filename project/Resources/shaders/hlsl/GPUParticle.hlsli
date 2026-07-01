struct GPUParticle
{
    float3 translate;
    float lifeTime;
    float3 scale;
    float maxTime;
    float3 startScale;
    float angularVelocity;
    float3 velocity;
    float effectType;
    float3 acceleration;
    float isAlive;
    float4 color;
    float rotateZ;
    float3 pad;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

static const uint kMaxParticles = 1024;

struct EmitterSphere
{
    float3 translate;
    float radius;
    uint count;
    float frequency;
    float frequencyTime;
    uint emit;
    float effectType;
    float sizeMultiplier;
    float2 emitterPad;
};

struct PerFrame
{
    float time;
    float deltaTime;
    float2 perFramePad;
};

// 簡易乱数生成器。Generate3d()を呼ぶたびにseedも更新する。
class RandomGenerator
{
    float3 seed;

    float3 Generate3d()
    {
        seed = frac(sin(float3(
            dot(seed, float3(12.9898f, 78.233f, 37.719f)),
            dot(seed, float3(39.3468f, 11.135f, 83.155f)),
            dot(seed, float3(73.156f, 52.235f, 9.151f)))) * 43758.5453f);
        return seed;
    }

    float Generate1d()
    {
        float value = Generate3d().x;
        return value;
    }
};

float Random1d(inout float3 seed)
{
    float result =
        frac(
            sin(
                dot(
                    seed,
                    float3(
                        12.9898f,
                        78.233f,
                        37.719f
                    )
                )
            ) *
            43758.5453f
        );

    seed =
        float3(
            result,
            frac(result * 17.0f),
            frac(result * 59.0f)
        );

    return result;
}

float3 Random3d(inout float3 seed)
{
    float x = Random1d(seed);
    float y = Random1d(seed);
    float z = Random1d(seed);

    return float3(x, y, z);
}