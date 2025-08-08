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

struct MeshMatrices
{
    matrix WorldMatrix;
    matrix ViewMatrix;
    matrix ProjectionMatrix;
    bool IsActiveAnimations;
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
};


#endif