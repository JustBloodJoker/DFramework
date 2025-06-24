struct VS_IN {
    float4 pos : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VS_OUT{
    float4 pos : SV_Position;
    float2 texCoord : TEXCOORD;
};

Texture2D inputTexture: register(t0);
SamplerState samplerState : register(s0);

VS_OUT mainVS(VS_IN input) {
    VS_OUT output;
    output.pos = input.pos;
    output.texCoord = input.texCoord / input.pos.z;
    return output;
}

float4 mainPS(VS_OUT input) : SV_Target {
    return inputTexture.Sample(samplerState, input.texCoord);
}  