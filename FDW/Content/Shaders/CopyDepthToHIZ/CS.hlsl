Texture2D<float> DepthTex : register(t0);
RWTexture2D<float> OutTex : register(u0);

[numthreads(8,8,1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{
    float d = DepthTex.Load(int3(dtid.xy, 0));
    OutTex[dtid.xy] = d;
}