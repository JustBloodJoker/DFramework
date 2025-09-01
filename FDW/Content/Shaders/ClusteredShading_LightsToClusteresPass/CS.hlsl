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
    float3 center = mul(float4(Lights[lightIndex].Position, 1.0f), ViewMatrix).xyz;
    float radius = Lights[lightIndex].AttenuationRadius;
    return SphereAABBIntersection(center, radius, c.MinPoint.xyz, c.MaxPoint.xyz);
}

bool TestDirectionalLight(uint lightIndex, Cluster c)
{
    return true;
}

#define RECT_LIGHT_MIN_LIGHT_COEF  1e-7
float ComputeRectInfluenceRadius(LightStruct light)
{
    float area = light.RectSize.x * light.RectSize.y;
    float luminance = max(light.Color.r, max(light.Color.g, light.Color.b));
    float radius = sqrt((light.Intensity * area * luminance) / (4.0 * PI * RECT_LIGHT_MIN_LIGHT_COEF));
    return radius;
}

bool TestRectLight(uint lightIndex, Cluster c)
{
    LightStruct light = Lights[lightIndex];

    float3 points[4];
    InitRectPoints(light, points);
    float3 lightNormal = normalize(cross(points[1] - points[0], points[3] - points[0]));
    float3 clusterCenter = 0.5 * (c.MinPoint.xyz + c.MaxPoint.xyz);
    float3 toCluster = points[0].xyz - clusterCenter;
    if (dot(toCluster, lightNormal) < 0.0f)
        return false;

    float3 minPt = min(min(points[0], points[1]), min(points[2], points[3]));
    float3 maxPt = max(max(points[0], points[1]), max(points[2], points[3]));

    float influenceRadius = ComputeRectInfluenceRadius(light);

    minPt -= influenceRadius;
    maxPt += influenceRadius;

    float3 cMin = c.MinPoint.xyz;
    float3 cMax = c.MaxPoint.xyz;

    return (minPt.x <= cMax.x && maxPt.x >= cMin.x) &&
           (minPt.y <= cMax.y && maxPt.y >= cMin.y) &&
           (minPt.z <= cMax.z && maxPt.z >= cMin.z);
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
