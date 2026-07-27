struct Vertex
{
    float4 position;
    float2 texcoord;
    float3 normal;
};

struct VertexInfluence
{
    float4 weights;
    int4 jointIndices;
};

struct Well
{
    float4x4 skeletonSpaceMatrix;
    float4x4 skeletonSpaceInverseTransposeMatrix;
};

StructuredBuffer<Vertex> gInputVertices : register(t0);
StructuredBuffer<VertexInfluence> gInfluences : register(t1);
StructuredBuffer<Well> gMatrixPalette : register(t2);
RWStructuredBuffer<Vertex> gOutputVertices : register(u0);

cbuffer SkinningInformation : register(b0)
{
    uint gVertexCount;
};

[numthreads(1024, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    const uint vertexIndex = dispatchThreadId.x;
    if (vertexIndex >= gVertexCount)
    {
        return;
    }

    const Vertex input = gInputVertices[vertexIndex];
    const VertexInfluence influence = gInfluences[vertexIndex];

    float4 skinnedPosition = 0.0f;
    float3 skinnedNormal = 0.0f;

    [unroll]
    for (uint influenceIndex = 0; influenceIndex < 4; ++influenceIndex)
    {
        const float weight = influence.weights[influenceIndex];
        if (weight <= 0.0f)
        {
            continue;
        }

        const Well joint = gMatrixPalette[
            influence.jointIndices[influenceIndex]
        ];
        skinnedPosition +=
            mul(input.position, joint.skeletonSpaceMatrix) * weight;
        skinnedNormal +=
            mul(
                input.normal,
                (float3x3)joint.skeletonSpaceInverseTransposeMatrix
            ) * weight;
    }

    Vertex output;
    output.position = float4(skinnedPosition.xyz, 1.0f);
    output.texcoord = input.texcoord;
    output.normal = normalize(skinnedNormal);
    gOutputVertices[vertexIndex] = output;
}
