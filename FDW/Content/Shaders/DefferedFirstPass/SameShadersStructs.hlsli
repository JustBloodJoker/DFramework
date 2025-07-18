struct VERTEX_OUTPUT
{
    float4 pos : SV_Position;
    float3 worldPos : POSITION0;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    uint instance : SV_InstanceID;
};