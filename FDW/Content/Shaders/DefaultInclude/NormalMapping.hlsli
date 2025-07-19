struct NMInputData{
    float2 TextureCoords;
    float3x3 TBN;
    Texture2D NormalTexture;
    SamplerState Sampler;
};

float3 NormalMapping(NMInputData inData){
    float3 normalMapSample = inData.NormalTexture.Sample(inData.Sampler,  inData.TextureCoords).rgb;
    normalMapSample = normalize(normalMapSample * 2.0f - 1.0f);
    return normalize( mul(normalMapSample, inData.TBN) );
}