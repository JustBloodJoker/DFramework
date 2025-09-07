#ifndef _STRUCTURES_HLSLI_
#define _STRUCTURES_HLSLI_

struct VERTEX_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct STATICVERTEX_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct Matrices
{
    matrix WorldMatrix;
    matrix ViewMatrix;
    matrix ProjectionMatrix;
};

struct ViewProjectionMatrices
{
    matrix ViewMatrix;
    matrix ProjectionMatrix;
};

struct ViewProjectionMatrices_Mul
{
    matrix ViewMatrix;
    matrix ProjectionMatrix;
    matrix ViewProjectionMatrix;
};

struct MeshMatrices
{
    matrix WorldMatrix;
    matrix ViewMatrix;
    matrix ProjectionMatrix;
    int IsActiveAnimations;
    float3 CameraPosition;
};

#define TEXTURE_BASE_LOAD_FLAG_LOCATION             0
#define TEXTURE_NORMAL_LOAD_FLAG_LOCATION           1
#define TEXTURE_ROUGHNESS_LOAD_FLAG_LOCATION        2
#define TEXTURE_METALNESS_LOAD_FLAG_LOCATION        3
#define TEXTURE_HEIGHT_LOAD_FLAG_LOCATION           4
#define TEXTURE_SPECULAR_LOAD_FLAG_LOCATION         5
#define TEXTURE_OPACITY_LOAD_FLAG_LOCATION          6
#define TEXTURE_AMBIENT_LOAD_FLAG_LOCATION          7
#define TEXTURE_EMISSIVE_LOAD_FLAG_LOCATION         8

#define TEXTURE_ORM_TEXTURE_TYPE_FLAG_LOCATION   9

struct Materials
{
    float4 diffuse;
    float4 ambient;
    float4 emissive;
    float4 specular;
    float roughness;
    float metalness;
    float specularPower;
    float heightScale;
    int4 LoadedTexture[3];
};

struct LightsHelper{
    int LightCount;
    float3 CameraPos;
    int IsShadowImpl;
    float3 margin;
};


#define LIGHT_POINT_LIGHT_ENUM_VALUE 0
#define LIGHT_SPOT_LIGHT_ENUM_VALUE 1
#define LIGHT_DIRECTIONAL_LIGHT_ENUM_VALUE 2
#define LIGHT_RECT_LIGHT_ENUM_VALUE 3


struct LightStruct {
    int LightType;
    float3 Color;
    float Intensity;

    float3 Position;

    //Point light fields
    float AttenuationRadius;
    float SourceRadius;

    //SpotLight
    float3 Direction; //Directional Light
    float InnerConeAngle;
    float OuterConeAngle;

    //RectLight
    float2 RectSize;
    float3 Rotation;
};

struct RTSoftShadowFrameStruct {
    matrix PrevViewProj;
    matrix CurrViewProj;
    float TemporalFeedbackMin;
    float TemporalFeedbackMax;
    float ReprojDistThreshold;
    float NormalThreshold;
    uint FrameIndex;
    int3 padding;
};

struct AABB
{
    float3 MinV;
    float3 MaxV;
};

struct Frustum { 
    float4 Planes[6]; 
    float3 Corners[8]; 
};

struct Frustum_ObjectCulling
{
    float4 Planes[6];
};

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

struct OcclusionDataStruct {
    Frustum_ObjectCulling Frustum;
    matrix ViewProjection;
    uint InstanceCount;
    int MipLevels;
    int HiZWidth;
    int HiZHeight;
};

#define MAX_LIGHTS_PER_CLUSTER 119
struct Cluster
{
    float4 MinPoint;
    float4 MaxPoint;
    uint Count;
    uint LightIndices[MAX_LIGHTS_PER_CLUSTER];
};

struct ClusterParams
{
    float ZNear;
    float ZFar;
    uint GridSize0;
    uint GridSize1;
    uint GridSize2;
    uint ScreenWidth;
    uint ScreenHeight;
    uint padd;
    float4x4 InverseProjection;
};

struct ClusterParamsPS {
    float ZNear;
    float ZFar;
    uint GridSize0;
    uint GridSize1;
    uint GridSize2;
    uint ScreenWidth;
    uint ScreenHeight;
    uint padd;
    float4x4 ViewMatrix;
};

struct BrightPassData {
    float Threshold;
    float3 padd;
};

struct BlurParams {
    float2 TexelSize;
    int Horizontal;   // 1 = horizontal pass, 0 = vertical pass
    int padd;
};

struct BloomCompositeData{
    float BloomIntensity;
    float3 padd;
};

#endif