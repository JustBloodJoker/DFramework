#ifndef _STRUCTURES_HLSLI_
#define _STRUCTURES_HLSLI_

struct MeshMatrices
{
    matrix WorldMatrix;
    matrix ViewMatrix;
    matrix ProjectionMatrix;

    float3 CameraPosition;
    float Padding;
};

#define TEXTURE_BASE_LOAD_FLAG_LOCATION             0
#define TEXTURE_NORMAL_LOAD_FLAG_LOCATION           1
#define TEXTURE_HEIGHT_LOAD_FLAG_LOCATION           4
#define MATERIAL_ALPHA_MODE_FLAG_LOCATION           9

#define MATERIAL_ALPHA_MODE_OPAQUE               0

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

    // Versioned shadow-parameter extension (Light layout ).
    float DirectionalAngularRadius;
    float ShadowSoftness;
};

#define MAX_LIGHTS_PER_CLUSTER 300
struct Cluster
{
    float4 MinPoint;
    float4 MaxPoint;
    uint Count;
    uint AffectingLightCount;
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

struct LightAtlasMeta {
    uint LightIndex;
    uint AtlasOffsetX;
    uint AtlasOffsetY;
    uint AtlasWidth;
    uint AtlasHeight;
    uint padd1;
    uint padd2;
    uint padd3;
    float ScreenMinU;
    float ScreenMinV;
    float ScreenMaxU;
    float ScreenMaxV;
};

#define SHADOW_ATLAS_FILTER_MODE_NEAREST                     0
#define SHADOW_ATLAS_FILTER_MODE_BILINEAR                    1
#define SHADOW_ATLAS_FILTER_MODE_PCF                         2
#define SHADOW_ATLAS_FILTER_MODE_JOINT_BILATERAL             3
#define SHADOW_ATLAS_FILTER_MODE_JOINT_BILATERAL_PCF         4
#define SHADOW_ATLAS_FILTER_MODE_POISSON_16                  5
#define SHADOW_ATLAS_FILTER_MODE_JOINT_BILATERAL_POISSON_16  6

struct ShadowAtlasParams {
    uint ScreenWidth;
    uint ScreenHeight;
    uint AtlasWidth;
    uint AtlasHeight;
    uint FilterMode;
    uint PCFKernelSize;
    float FilterRadius;
    float DepthRejectionSharpness;
    float NormalRejectionSharpness;
    float BlackLevel;
    float ShadowContrast;
    float NoiseScale;
    matrix InverseViewProjectionMatrix;
};

#endif
