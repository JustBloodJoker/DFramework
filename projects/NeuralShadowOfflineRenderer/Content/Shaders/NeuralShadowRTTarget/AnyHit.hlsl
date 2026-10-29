struct ShadowPayload
{
    float Visible;
};

struct Attributes
{
    float2 bary;
};

struct NSRTGeometryInstance
{
    uint IndexOffset;
    uint VertexOffset;
    uint VertexStride;
    uint AlphaTextureIndex;
    float AlphaCutoff;
    float OpacityFactor;
    uint Flags;
    uint Padding;
};

static const uint NS_RT_GEOMETRY_ALPHA_TESTED = 1u;
static const uint NS_RT_VERTEX_TEXCOORD_OFFSET = 24u;

Texture2D<float4> GeometryTextures[200] : register(t6);
StructuredBuffer<NSRTGeometryInstance> GeometryInstances : register(t206);
ByteAddressBuffer GeometryIndices : register(t207);
ByteAddressBuffer GeometryVertices : register(t208);
SamplerState GeometrySampler : register(s0);

float2 LoadTexCoord(NSRTGeometryInstance instance, uint localVertex)
{
    uint byteOffset = (instance.VertexOffset + localVertex) * instance.VertexStride +
        NS_RT_VERTEX_TEXCOORD_OFFSET;
    return asfloat(GeometryVertices.Load2(byteOffset));
}

[shader("anyhit")]
void AnyHit(inout ShadowPayload payload, Attributes attributes)
{
    NSRTGeometryInstance instance = GeometryInstances[InstanceID()];
    if ((instance.Flags & NS_RT_GEOMETRY_ALPHA_TESTED) == 0u)
        return;

    uint triangleIndexOffset = (instance.IndexOffset + PrimitiveIndex() * 3u) * 4u;
    uint3 indices = uint3(
        GeometryIndices.Load(triangleIndexOffset),
        GeometryIndices.Load(triangleIndexOffset + 4u),
        GeometryIndices.Load(triangleIndexOffset + 8u));
    float3 barycentrics = float3(1.0f - attributes.bary.x - attributes.bary.y,
        attributes.bary.x, attributes.bary.y);
    float2 uv = LoadTexCoord(instance, indices.x) * barycentrics.x +
        LoadTexCoord(instance, indices.y) * barycentrics.y +
        LoadTexCoord(instance, indices.z) * barycentrics.z;
    float alpha = GeometryTextures[NonUniformResourceIndex(instance.AlphaTextureIndex)]
        .SampleLevel(GeometrySampler, uv, 0.0f).a * instance.OpacityFactor;
    if (!isfinite(alpha) || alpha < instance.AlphaCutoff)
        IgnoreHit();
}
