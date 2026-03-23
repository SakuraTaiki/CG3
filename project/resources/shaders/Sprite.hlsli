// ========================================
// 型定義（objectと揃える）
// ========================================
typedef float float32_t;
typedef float2 float32_t2;
typedef float3 float32_t3;
typedef float4 float32_t4;
typedef float4x4 float32_t4x4;

// ========================================
// VS→PS構造体
// ========================================
struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION0;
    float32_t2 texcoord : TEXCOORD0;
};
