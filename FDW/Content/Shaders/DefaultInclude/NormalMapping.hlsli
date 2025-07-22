struct NMInputData{
    float2 TextureCoords;
    float3x3 TBN;
    Texture2D NormalTexture;
    SamplerState Sampler;
};

float3 NormalMapping(NMInputData inData){
    float3 normalMap = inData.NormalTexture.Sample( inData.Sampler, inData.TextureCoords).rgb;
    normalMap = (2.0f * normalMap) - 1.0f;
    return normalize(mul(normalMap.rgb, inData.TBN));
}