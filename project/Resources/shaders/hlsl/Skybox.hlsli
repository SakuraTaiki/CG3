struct VertexShaderInput
{
    float32_t4 position : POSITION0;
};

struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t3 texcoord : TEXCOORD0;
};

struct TransformationMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 World;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);