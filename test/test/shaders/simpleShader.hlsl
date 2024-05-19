



struct VERTEX_INPUT
{

    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct VERTEX_OUTPUT
{
    float4 pos : SV_Position;
    float3 worldPos : POSITION0;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0 ;
};

struct Matrices
{
    matrix WorldMatrix;
    matrix ViewMatrix;
    matrix ProjectionMatrix;
};

struct Materials
{
    float4 diffuse;
    float4 ambient;
    float4 emissive;
    float4 specular;
};

ConstantBuffer<Matrices> objMatrices : register(b0);
ConstantBuffer<Materials> materialsObject : register(b1);

SamplerState ss : register(s0);
Texture2D tex : register(t0);

VERTEX_OUTPUT VS(VERTEX_INPUT vsIn)
{
    VERTEX_OUTPUT vsOut;
    vsOut.pos = mul(float4(vsIn.pos, 1.0f), objMatrices.WorldMatrix);
    
    vsOut.worldPos = vsOut.pos.xyz;
    
    vsOut.pos = mul(vsOut.pos, objMatrices.ViewMatrix);
    vsOut.pos = mul(vsOut.pos, objMatrices.ProjectionMatrix);

    vsOut.texCoord = vsIn.texCoord;
    vsOut.normal = normalize(vsIn.normal);
    vsOut.tangent = vsIn.tangent;
    vsOut.bitangent = vsIn.bitangent;

    return vsOut;
}

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    psOut.result = tex.Sample( ss, vsOut.texCoord );//materialsObject.diffuse;
    clip(psOut.result.a < 0.1f ? -1 : 1);


    return psOut;

}
