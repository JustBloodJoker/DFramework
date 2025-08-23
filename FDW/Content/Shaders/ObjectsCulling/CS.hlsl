
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

struct OcclusionDataStruct {
    Frustum Frustum;
    matrix ViewProjection;
    uint InstanceCount;
    int MipLevels;
    int HiZWidth;
    int HiZHeight;
};

ConstantBuffer<OcclusionDataStruct> OcclusionData : register(b0);

#define OCCLUSION_BIAS 1e-3

//IN
StructuredBuffer<InstanceGPU> Instances : register(t0);
StructuredBuffer<IndirectMeshCommand> InputCommands : register(t1);
Texture2D<float> HIZTexture : register(t2);
SamplerState HIZSampler : register(s0);

//OUT
AppendStructuredBuffer<IndirectMeshCommand> OutputCommands : register(u0);

bool IntersectFrustum(InstanceGPU inst, Frustum frustum)
{
    float3 center = 0.5f * (inst.MinP + inst.MaxP);
    float3 extents = 0.5f * (inst.MaxP - inst.MinP);

    [unroll]
    for (int i = 0; i < 6; i++)
    {
        float4 plane = frustum.planes[i];
        float3 normal = plane.xyz;
        float dist = dot(normal, center) + plane.w;
        float radius = dot(abs(normal), extents);

        if (dist + radius < 0)
            return false;
    }
    return true;
}

int GetHiZMipLevel(float2 minScreen, float2 maxScreen, int MipLevelsCount, int hizWidth, int hizHeight)
{
    float2 clampedMin = max(minScreen, float2(0.0f, 0.0f));
    float2 clampedMax = min(maxScreen, float2(hizWidth - 1, hizHeight - 1));

    float2 boxSize = clampedMax - clampedMin;

    float maxDimension = max(boxSize.x, boxSize.y);
    
    int mipLevel = floor(log2(max(1.0f, maxDimension)));
    
    return min(mipLevel, MipLevelsCount);
}

bool IntersectHIZ(InstanceGPU inst, int MipLevelsCount, matrix vp, int hizWidth, int hizHeight, Texture2D<float> hizTexture, SamplerState hizSampler)
{
    float3 corners[8];
    corners[0] = float3(inst.MinP.x, inst.MinP.y, inst.MinP.z);
    corners[1] = float3(inst.MaxP.x, inst.MinP.y, inst.MinP.z);
    corners[2] = float3(inst.MinP.x, inst.MaxP.y, inst.MinP.z);
    corners[3] = float3(inst.MaxP.x, inst.MaxP.y, inst.MinP.z);
    corners[4] = float3(inst.MinP.x, inst.MinP.y, inst.MaxP.z);
    corners[5] = float3(inst.MaxP.x, inst.MinP.y, inst.MaxP.z);
    corners[6] = float3(inst.MinP.x, inst.MaxP.y, inst.MaxP.z);
    corners[7] = float3(inst.MaxP.x, inst.MaxP.y, inst.MaxP.z);

    float2 minScreen = float2(1e5f, 1e5f);
    float2 maxScreen = float2(-1e5f, -1e5f);
    float minClipZ = 1.0f;

    [unroll]
    for (int i = 0; i < 8; ++i)
    {
        float4 clipPos = mul(float4(corners[i], 1.0f), vp);
        
        float3 ndc = clipPos.xyz / clipPos.w;

        float2 screenPos = float2((ndc.x * 0.5f + 0.5f) * hizWidth, (-ndc.y * 0.5f + 0.5f) * hizHeight);

        minScreen = min(minScreen, screenPos);
        maxScreen = max(maxScreen, screenPos);
        minClipZ = min(minClipZ, ndc.z);
    }

    int mipLevel = GetHiZMipLevel(minScreen, maxScreen, MipLevelsCount, hizWidth, hizHeight);
    float4 minHizTexel = hizTexture.SampleLevel(hizSampler, minScreen / float2(hizWidth, hizHeight), mipLevel);
    float4 maxHizTexel = hizTexture.SampleLevel(hizSampler, maxScreen / float2(hizWidth, hizHeight), mipLevel);

    float hizDepth = min(minHizTexel.r, maxHizTexel.r);
    bool visible = (minClipZ <= hizDepth + OCCLUSION_BIAS);
    return visible;
}


[numthreads(64,1,1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint idx = dispatchThreadID.x;
    if (idx >= OcclusionData.InstanceCount) return;

    InstanceGPU inst = Instances[idx];

    if (!IntersectFrustum(inst, OcclusionData.Frustum)) return;
    
    if (OcclusionData.MipLevels != -1)
    {
        if (!IntersectHIZ(inst, OcclusionData.MipLevels, OcclusionData.ViewProjection, OcclusionData.HiZWidth, OcclusionData.HiZHeight, HIZTexture, HIZSampler)) return;
    }
    
    IndirectMeshCommand cmd = InputCommands[inst.CommandIndex];
    OutputCommands.Append(cmd);
}