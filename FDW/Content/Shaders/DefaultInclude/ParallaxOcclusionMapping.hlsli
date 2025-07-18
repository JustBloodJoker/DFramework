struct POMInputData {
    float3 WorldPosition;
    float3 CameraPosition;
    float3x3 TBN;
    float HeightScale;
    float2 TextureCoords;
    Texture2D HeightTexture;
    SamplerState Sampler;
};


float2 ParallaxOcclusionMapping(POMInputData inData)
{
    const float minLayers = 8.0;
    const float maxLayers = 32.0;

    float2 dx = ddx( inData.TextureCoords );
    float2 dy = ddy( inData.TextureCoords );

    float3 viewDirWS = normalize(inData.CameraPosition - inData.WorldPosition);
    float3 viewDirTS = mul(viewDirWS, inData.TBN);

    float ndotv = abs(dot(float3(0.0, 0.0, 1.0), normalize(viewDirTS)));
    float numLayers = lerp(maxLayers, minLayers, ndotv);

    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;

    float2 P = viewDirTS.xy / viewDirTS.z * inData.HeightScale;
    float2 deltaTexCoord = P / numLayers;

    float2 currentTexCoord = inData.TextureCoords;
    float currentDepth = inData.HeightTexture.Sample(inData.Sampler, currentTexCoord).r;

    while (currentLayerDepth < currentDepth)
    {
        currentTexCoord -= deltaTexCoord;
        currentDepth = inData.HeightTexture.SampleGrad(inData.Sampler, currentTexCoord, dx, dy).r;
        currentLayerDepth += layerDepth;
    }

    float2 prevTexCoords = currentTexCoord + deltaTexCoord;
    float afterDepth  = currentDepth - currentLayerDepth;
    float beforeDepth = inData.HeightTexture.Sample(inData.Sampler, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    float weight = afterDepth / (afterDepth - beforeDepth);
    float2 finalTexCoords = prevTexCoords * weight + currentTexCoord * (1.0 - weight);

    return finalTexCoords;
}