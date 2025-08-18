
struct InstanceGPU
{
    float3 MaxP;
    uint padd;
    float3 MinP;
    uint CommandIndex;
};

struct IndirectMeshCommand{
    uint2 CBMatricesAddress;
    uint2 CBMaterialsAddress;
    uint2 SRVBonesAddress;
    //Vertex buffer view
    uint2 VBVAddress;
    uint VBVSizeInBytes;
    uint VBVStrideInBytes;
    //Index buffer view
    uint2 IBVAddress;
    uint IBVSizeInBytes;
    uint IBVFormat;
    //Draw args indexed
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int BaseVertexLocation;
    uint StartInstanceLocation;
    uint padd;

};

struct Frustum
{
    float4 planes[6];
};

cbuffer CameraCB : register(b0)
{
    Frustum frustum;
    uint InstanceCount;
    uint pad0;
    uint pad1;
    uint pad2;
};


StructuredBuffer<InstanceGPU> Instances : register(t0);
StructuredBuffer<IndirectMeshCommand> InputCommands : register(t1);
AppendStructuredBuffer<IndirectMeshCommand> OutputCommands : register(u0);
bool IsVisible(float3 maxPoint, float3 minPoint, Frustum frustum)
{
    [unroll]
    for (int i = 0; i < 6; i++) {
        float4 p = frustum.planes[i];

        float3 positive;
        positive.x = (p.x >= 0.0f) ? maxPoint.x : minPoint.x;
        positive.y = (p.y >= 0.0f) ? maxPoint.y : minPoint.y;
        positive.z = (p.z >= 0.0f) ? maxPoint.z : minPoint.z;

        float dist = p.x * positive.x + p.y * positive.y + p.z * positive.z + p.w;

        if (dist < 0.0f) return false;
    }

    return true;
}


[numthreads(64, 1, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint idx = dispatchThreadID.x;
    if (idx >= InstanceCount) return;

    InstanceGPU inst = Instances[idx];

    if (IsVisible(inst.MaxP, inst.MinP, frustum))
    {
        IndirectMeshCommand cmd = InputCommands[inst.CommandIndex];
        OutputCommands.Append(cmd);
    }
}