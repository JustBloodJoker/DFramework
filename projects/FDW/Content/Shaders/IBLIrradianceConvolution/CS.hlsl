#include "Utilits.hlsli"
#include "IBLFunctions.hlsli"

TextureCube<float4> EnvironmentMap : register(t0);
RWTexture2DArray<float4> IrradianceMap : register(u0);
SamplerState LinearWrapSampler : register(s0);

[numthreads(8, 8, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{
    uint width, height, layers;
    IrradianceMap.GetDimensions(width, height, layers);

    if (dtid.x >= width || dtid.y >= height || dtid.z >= layers) return;

    float2 uv = (float2(dtid.xy) + 0.5f) / float2(width, height);
    float nx = uv.x * 2.0f - 1.0f;
    float ny = 1.0f - uv.y * 2.0f;

    float3 N = CubemapDirectionFromFaceUV(dtid.z, nx, ny);
    float3 up = (abs(N.z) < 0.999f) ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    const uint SAMPLE_COUNT = 64u;
    float3 irradiance = 0.0f;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        float2 xi = Hammersley(i, SAMPLE_COUNT);
        float3 localL = SampleCosineHemisphere(xi);
        float3 L = normalize(localL.x * tangent + localL.y * bitangent + localL.z * N);
        irradiance += EnvironmentMap.SampleLevel(LinearWrapSampler, L, 0.0f).rgb;
    }

    irradiance *= (PI / float(SAMPLE_COUNT));
    IrradianceMap[uint3(dtid.xy, dtid.z)] = float4(irradiance, 1.0f);
}
