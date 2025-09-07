#include "Structures.hlsli"
#include "LTCFunctions.hlsli"

RWStructuredBuffer<Cluster> Clusters : register(u0);
StructuredBuffer<LightStruct> Lights : register(t0);

cbuffer ViewParams : register(b0)
{
    float4x4 ViewMatrix;
    int LightCount;
};

bool SphereAABBIntersection(float3 center, float radius, float3 aabbMin, float3 aabbMax);
bool TestPointLight(uint lightIndex, Cluster c);
bool TestSpotLight(uint lightIndex, Cluster c);
bool TestDirectionalLight(uint lightIndex, Cluster c);
bool TestRectLight(uint lightIndex, Cluster c);

bool TestPointLight(uint lightIndex, Cluster c)
{
    float3 center = mul(float4(Lights[lightIndex].Position, 1.0f), ViewMatrix).xyz;
    float radius = Lights[lightIndex].AttenuationRadius;

    return SphereAABBIntersection(center, radius, c.MinPoint.xyz, c.MaxPoint.xyz);
}

bool TestSpotLight(uint lightIndex, Cluster c)
{
    return true;
    float3 center = mul(float4(Lights[lightIndex].Position, 1.0f), ViewMatrix).xyz;
    float radius = Lights[lightIndex].AttenuationRadius;
    return SphereAABBIntersection(center, radius, c.MinPoint.xyz, c.MaxPoint.xyz);
}

bool TestDirectionalLight(uint lightIndex, Cluster c)
{
    return true;
}

bool TestRectLight(uint lightIndex, Cluster c)
{
    return true;
}


bool SphereAABBIntersection(float3 center, float radius, float3 aabbMin, float3 aabbMax)
{
    float3 closest = clamp(center, aabbMin, aabbMax);
    float3 diff = closest - center;
    return dot(diff, diff) <= radius * radius;
}

#define THREAD_X_SIZE 128

[numthreads(THREAD_X_SIZE, 1, 1)]
void CS(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    uint index = groupID.x * THREAD_X_SIZE + groupThreadID.x;
    Cluster c = Clusters[index];
    c.Count = 0;

    for (uint i = 0; i < LightCount; i++)
    {
        bool addd = false;
        int type = Lights[i].LightType;

        if (type == LIGHT_POINT_LIGHT_ENUM_VALUE) addd = TestPointLight(i, c);
        else if (type == LIGHT_SPOT_LIGHT_ENUM_VALUE) addd = TestSpotLight(i, c);
        else if (type == LIGHT_DIRECTIONAL_LIGHT_ENUM_VALUE) addd = TestDirectionalLight(i, c);
        else if (type == LIGHT_RECT_LIGHT_ENUM_VALUE) addd = TestRectLight(i, c);

        if (addd && c.Count < 100)
        {
            c.LightIndices[c.Count] = i;
            c.Count++;
        }
    }

    Clusters[index] = c;
}
