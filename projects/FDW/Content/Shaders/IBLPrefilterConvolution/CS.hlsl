#include "Utilits.hlsli"
#include "IBLFunctions.hlsli"

TextureCube<float4> EnvironmentMap : register(t0);
RWTexture2DArray<float4> PrefilteredMap : register(u0);
SamplerState LinearWrapSampler : register(s0);

cbuffer PrefilterParams : register(b0)
{
    float Roughness;
};

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    return a2 / max(PI * denom * denom, 1e-5f);
}

float3 ImportanceSampleGGX(float2 xi, float3 N, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0f * PI * xi.x;
    float cosTheta = sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
    float sinTheta = sqrt(max(1.0f - cosTheta * cosTheta, 0.0f));

    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    float3 up = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

[numthreads(8, 8, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{
    uint width, height, layers;
    PrefilteredMap.GetDimensions(width, height, layers);

    if (dtid.x >= width || dtid.y >= height || dtid.z >= layers) return;

    float2 uv = (float2(dtid.xy) + 0.5f) / float2(width, height);
    float nx = uv.x * 2.0f - 1.0f;
    float ny = 1.0f - uv.y * 2.0f;

    float3 N = CubemapDirectionFromFaceUV(dtid.z, nx, ny);
    float3 V = N;

    uint envWidth, envHeight, envMipLevels;
    EnvironmentMap.GetDimensions(0u, envWidth, envHeight, envMipLevels);

    float sourceResolution = max(max(float(envWidth), float(envHeight)), 1.0f);
    float saTexel = 4.0f * PI / (6.0f * sourceResolution * sourceResolution);

    const uint SAMPLE_COUNT = 256u;
    float3 prefilteredColor = 0.0f;
    float totalWeight = 0.0f;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        float2 xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(xi, N, max(Roughness, 0.001f));
        float3 L = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = saturate(dot(N, L));
        if (NdotL > 0.0f) {
            float NdotH = saturate(dot(N, H));
            float HdotV = saturate(dot(H, V));

            float D = DistributionGGX(N, H, max(Roughness, 0.001f));
            float pdf = max((D * NdotH) / max(4.0f * HdotV, 1e-5f), 1e-5f);

            float saSample = 1.0f / (float(SAMPLE_COUNT) * pdf + 1e-5f);
            float mipLevel = (Roughness <= 0.0001f) ? 0.0f : 0.5f * log2(saSample / saTexel);
            mipLevel = clamp(mipLevel, 0.0f, max(float(envMipLevels) - 1.0f, 0.0f));

            prefilteredColor += EnvironmentMap.SampleLevel(LinearWrapSampler, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor /= max(totalWeight, 1e-5f);
    PrefilteredMap[uint3(dtid.xy, dtid.z)] = float4(prefilteredColor, 1.0f);
}
