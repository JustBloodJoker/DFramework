#ifndef _LTC_FUNCTIONS_HLSLI_
#define _LTC_FUNCTIONS_HLSLI_

#include "Structures.hlsli"
#include "Utilits.hlsli"

#define LUT_SIZE (64.0)
#define LUT_SCALE ((LUT_SIZE - 1.0)/LUT_SIZE)
#define LUT_BIAS (0.5/LUT_SIZE)

void InitRectPoints(LightStruct light, out float3 points[4])
{
    float3 dirx = RotateAxisQuat(float3(1, 0, 0), light.Rotation.x, light.Rotation.y, light.Rotation.z);
    float3 diry = RotateAxisQuat(float3(0, 1, 0), light.Rotation.x, light.Rotation.y, light.Rotation.z);
    
    float3 center = light.Position;
    float halfx = 0.5 * light.RectSize.x;
    float halfy = 0.5 * light.RectSize.y;

    float3 ex = halfx * dirx;
    float3 ey = halfy * diry;

    points[0] = center - ex - ey;
    points[1] = center + ex - ey;
    points[2] = center + ex + ey;
    points[3] = center - ex + ey;
}

float3 IntegrateEdgeVec(float3 v1, float3 v2)
{
    float x = dot(v1, v2);
    float y = abs(x);

    float a = 0.8543985 + (0.4965155 + 0.0145206 * y) * y;
    float b = 3.4175940 + (4.1616724 + y) * y;
    float v = a / b;

    float theta_sintheta = (x > 0.0) ? v : 0.5 * rsqrt(max(1.0 - x * x, 1e-7)) - v;

    return cross(v1, v2) * theta_sintheta;
}

float3 LTCEvaluate(float3 N, float3 V, float3 P, float3x3 Minv, float3 points[4])
{
    float3 T1, T2;
    T1 = normalize(V - N * dot(V, N));
    T2 = cross(N, T1);

    Minv = mul(Minv, (float3x3(T1, T2, N)));

    float3 L[5];
    L[0] = mul(Minv, points[0] - P);
    L[1] = mul(Minv, points[1] - P);
    L[2] = mul(Minv, points[2] - P);
    L[3] = mul(Minv, points[3] - P);

    float sum = 0.0;
    float3 dir = points[0].xyz - P;
    float3 lightNormal = cross(points[1] - points[0], points[3] - points[0]);
    bool behind = (dot(dir, lightNormal) < 0.0);

    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);

    float3 vsum = float3(0.0, 0, 0);

    vsum += IntegrateEdgeVec(L[0], L[1]);
    vsum += IntegrateEdgeVec(L[1], L[2]);
    vsum += IntegrateEdgeVec(L[2], L[3]);
    vsum += IntegrateEdgeVec(L[3], L[0]);
    
    float l = length(vsum);
    sum = max((l * l + vsum.z) / (l + 1), 0.0f);
    
    if (behind) sum = 0.0;

    float3 Lo_i = float3(sum, sum, sum);

    return (Lo_i / 2 * PI);
}

#endif