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

struct MeshMatrices
{
    matrix WorldMatrix;
    matrix ViewMatrix;
    matrix ProjectionMatrix;
    bool IsActiveAnimations;
};

#define TEXTURE_BASE_LOAD_FLAG_LOCATION             0
#define TEXTURE_NORMAL_LOAD_FLAG_LOCATION           1
#define TEXTURE_ROUGHNESS_LOAD_FLAG_LOCATION        2
#define TEXTURE_METALNESS_LOAD_FLAG_LOCATION        3
#define TEXTURE_HEIGHT_LOAD_FLAG_LOCATION           4
#define TEXTURE_SPECULAR_LOAD_FLAG_LOCATION         5
#define TEXTURE_OPACITY_LOAD_FLAG_LOCATION          6
#define TEXTURE_BUMP_LOAD_FLAG_LOCATION             7
#define TEXTURE_EMISSIVE_LOAD_FLAG_LOCATION         8

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

    bool LoadedTexture[9];
};