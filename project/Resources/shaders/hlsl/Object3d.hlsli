

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
    float32_t4 weight : WEIGHT0;
    int32_t4 index : INDEX0;
};


struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
    float32_t3 worldPosition : POSITION;
};

struct TransformationMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 World;
    float32_t4x4 WorldInverseTranspose;
};


struct Well
{
    float32_t4x4 skeletonSpaceMatrix;
    float32_t4x4 skeletonSpaceInverseTransposeMatrix;
};

struct Skinned
{
    float32_t4 position;
    float32_t3 normal;
};

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    
    float environmentCoefficient;
    float2 padding;
    
    float32_t4x4 uvTransform;
    
};

struct DirectionalLight
{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
};

struct CameraForGPU
{
    float32_t3 worldPosition;
};

struct PixelShaderOutput
{
    float32_t4 color : SV_Target0;
};

