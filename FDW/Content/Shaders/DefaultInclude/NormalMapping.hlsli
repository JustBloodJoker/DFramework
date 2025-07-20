struct NMInputData{
    float2 TextureCoords;
    float3x3 TBN;
    Texture2D NormalTexture;
    SamplerState Sampler;
};

float3 NormalMapping(NMInputData inData){
    float3 normalMap = inData.NormalTexture.Sample( inData.Sampler, inData.TextureCoords).rgb;
    
    // Convert from sRGB to linear space because the framework loads all textures in sRGB format,
    // but normal maps must be interpreted as linear to preserve correct direction data.
    normalMap = GammaCorrection(normalMap, 2.2);

    normalMap = (2.0f * normalMap) - 1.0f;
    return normalize(mul(normalMap.rgb, inData.TBN));
}