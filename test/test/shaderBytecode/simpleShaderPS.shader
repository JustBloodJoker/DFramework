DXBCЏщОр&њЁШџА    J     8         4  |    RDEFР              <   џџ     D%<      (   (   $                                                           џџџџ                  wraps tex Microsoft (R) HLSL Shader Compiler 10.1 ЋЋISGNј         А                    М                    Х                    Ь                   е                    н                    ч                    SV_Position POSITION NORMAL TEXCOORD TANGENT BITANGENT SV_InstanceID ЋЋЋOSGN,                               SV_TARGET ЋЋSHEX@  Q   P   j Z  Fn0                 X F~0             UU      b 2    e  ђ      h     E  ђ      F    6y           `          :  6        
      1       
      @  ЭЬЬ=7  	     
     @    П@    ?1       
     @             
     @  џџџџ 
     6  ђ           >  STAT   
                                                                                                                                           SPDB F  Microsoft C/C++ MSF 7.00
DS         #   М       "                                                                                                                                                                                                                                                                                                                                                                                                                                                                           Рџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџ8   јџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџџ       <       џџџџ                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         .1Ж f   ^Pq"MѕMфЈq8Mz-                          мQ3                                                                                                                                                                                                                                                                                                                                                                                                                                                                    ONES;
    float Weight[NUM_BONES_PER_VEREX] : WEIGHT_BONES;
};

struct VERTEX_OUTPUT
{
    float4 pos : SV_Position;
    float3 worldPos : POSITION0;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    uint instance : SV_InstanceID;
};


ConstantBuffer<Matrices> objMatrices : register(b0);

Texture2D tex : register(t0);
SamplerState wraps : register(s0);

StructuredBuffer<matrix> boneMatrices : register(t1);

ЦZ  Lш u Я ф   GЋ eЇ й* &k  *х P Г ­ б
 5Ў ЗЧ 8і ЪГ                                                                                                                                                                                                                                                                                                                                                                                                                                                     #include "Structures.hlsli"
#include "Utilits.hlsli"

#define NUM_BONES_PER_VEREX 13


struct ANIMVERTEX_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    uint IDs[NUM_BONES_PER_VEREX] : IDS_BONES;
    float Weight[NUM_BONES_PER_VEREX] : WEIGHT_BONES;
};

struct VERTEX_OUTPUT
{
    float4 pos : SV_Position;
    float3 worldPos : POSITION0;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    uint instance : SV_InstanceID;
};


ConstantBuffer<Matrices> objMatrices : register(b0);

Texture2D tex : register(t0);
SamplerState wraps : register(s0);

StructuredBuffer<matrix> boneMatrices : register(t1);

VERTEX_OUTPUT VS(ANIMVERTEX_INPUT vsIn, uint instance : SV_InstanceID )
{
    VERTEX_OUTPUT vsOut;
    
    matrix ResultWorldMatrix = objMatrices.WorldMatrix;
    for(int i = 0; i < NUM_BONES_PER_VEREX; i++)
    {
        ResultWorldMatrix += boneMatrices[vsIn.IDs[i]] * vsIn.Weight[i];
    }

    vsOut.pos = mul(float4(vsIn.pos, 1.0f), ResultWorldMatrix);
    vsOut.worldPos = vsOut.pos.xyz;
    vsOut.pos = mul(vsOut.pos, objMatrices.ViewMatrix);
    vsOut.pos = mul(vsOut.pos, objMatrices.ProjectionMatrix);
    vsOut.texCoord = vsIn.texCoord;
    vsOut.normal = normalize(vsIn.normal);
    vsOut.tangent = vsIn.tangent;
    vsOut.bitangent = vsIn.bitangent;
    vsOut.instance = instance;

    return vsOut;
}

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0 ;
};
PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    psOut.result = tex.Sample(wraps, vsOut.texCoord);
    AlphaClipping(psOut.result.a);
    return psOut;
}                                                                                                                                                                                                                                               struct VERTEX_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct Matrices
{
    matrix WorldMatrix;
    matrix ViewMatrix;
    matrix ProjectionMatrix;
};

struct Materials
{
    float4 diffuse;
    float4 ambient;
    float4 emissive;
    float4 specular;
};                                                                                                                void AlphaClipping(float alpha)
{
    clip(alpha < 0.1f ? -1.0f : 1.0f);
}                                                                                                                                                                                                                                                                                                                                                                                                                                                   ўяўя   Ї	   D:\Projects\DFramework\test\test\shaders\simpleShader.hlsl  d:\projects\dframework\test\test\shaders\simpleshader.hlsl Structures.hlsli structures.hlsli Utilits.hlsli utilits.hlsli #include "Structures.hlsli"
#include "Utilits.hlsli"

#define NUM_BONES_PER_VEREX 13


struct ANIMVERTEX_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    uint IDs[NUM_BONES_PER_VEREX] : IDS_Bт0и   ehEыЫк                                                        #       Ј   (   т0JЏ9M      <   Ј               =   (   т0J!      <   =                  (   т0мВж  x   <                                                                                                                                                                                                                                                                                                                             B <   
  4aJ
  4aJMicrosoft (R) HLSL Shader Compiler 10.1   2 =hlslFlags 0x5 hlslTarget ps_5_1 hlslEntry PS   *     L      р       р   	  `     PS   . >  	 vsOut                                  P     `    р      P    `    р     P    `    р     P    `    р     P    `    р     P    `    р     P    `    р     P    `    р      P     `    р $    P  $  `    р (    P  (  `    р 0    P  ,  `    р 4    P  0  `    р @    P  4  `    р D    P  8  `    р H    P  <  `    р P    P  @  `    р T    P  D  `    р X    P  H  `    р `   : >   <PS return value>                                  P     `    р      P    `    р     P    `    р     P    `    р    . >   psOut                                  P          Д     P         Д     P         Д     P         Д     2 M|   H      	&D 	
Є $	%D$  . >@    alpha                                  P      Є          N  є   H      NXЏд;@юЖи	   x   ЯO0ТЩ ДNWэ?АсЬ     Дk(Эо(ТќwID6%B  ђ           @         ќ   `   C  `   C      D     D      D     D   Є   D  Є   D   Р   D  Р   D   ф   D  ф   D      D     D     D    D   (  E  (  E   <  E  <  E    5  4  "  !  "  !  "  !  "  !  "  !  "  !  "  !         і            0                    4                                       
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         Ъ18        д   џџ   џџ     L   L      T        @       float4 ѓђё @       float3 ѓђё @       float2 ѓђё       pos ђё    worldPos ё    normal ѓђё   ( texCoord ё   0 tangent ђё   < bitangent  u   H instance ё"              L VERTEX_OUTPUT 
             result ѓђё"               PIXEL_OUTPUT ё
            
    @   
           
       т  ђё
     
          
 т  ђё
     2                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            VERTEX_OUTPUT VS(ANIMVERTEX_INPUT vsIn, uint instance : SV_InstanceID )
{
    VERTEX_OUTPUT vsOut;
    
    matrix ResultWorldMatrix = objMatrices.WorldMatrix;
    for(int i = 0; i < NUM_BONES_PER_VEREX; i++)
    {
        ResultWorldMatrix += boneMatrices[vsIn.IDs[i]] * vsIn.Weight[i];
    }

    vsOut.pos = mul(float4(vsIn.pos, 1.0f), ResultWorldMatrix);
    vsOut.worldPos = vsOut.pos.xyz;
    vsOut.pos = mul(vsOut.pos, objMatrices.ViewMatrix);
    vsOut.pos = mul(vsOut.pos, objMatrices.ProjectionMatrix);
    vsOut.texCoord = vsIn.texCoord;
    vsOut.normal = normalize(vsIn.normal);
    vsOut.tangent = vsIn.tangent;
    vsOut.bitangent = vsIn.bitangent;
    vsOut.instance = instance;

    return vsOut;
}

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0 ;
};
PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    psOut.result = tex.Sample(wraps, vsOut.texCoord);
    AlphaClipping(psOut.result.a);
    return psOut;
} struct VERTEX_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct Matrices
{
    matrix WorldMatrix;
    matrix ViewMatrix;
    matrix ProjectionMatrix;
};

struct Materials
{
    float4 diffuse;
    float4 ambient;
    float4 emissive;
    float4 specular;
}; void AlphaClipping(float alpha)
{
    clip(alpha < 0.1f ? -1.0f : 1.0f);
}                    <               Ж   =         Ш     x   Ј   Y	  
    Ъ18            џџ   џџ                            AlphaClipping ђё                                                                                                                                                                                                                                                                                                                                                                                                                                            D3DSHDR @                             `                    AlphaClipping ђё                                                                                                                                                                                                                                                                                                                                                                                                                                            џџџџ	/ё                 5                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           %    |    PS     d                   tex    d                   wraps                                                                                                                                                                                                                                                                                                                                                                                                                                                              нн    ннннџџџџ	/ё                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            .1Ж f   ^Pq"MѕMфЈq8Mz-   /LinkInfo /names /src/headerblock /src/files/d:\projects\dframework\test\test\shaders\simpleshader.hlsl /src/files/structures.hlsli /src/files/utilits.hlsli    
      ?          
         h   	          "      
          мQ3                                                                                                                                                                                                                                                       џџџџw	1      ?\   H       ,   p                                    @     `              P      |                 PS none -К.ё нн    @     `  нн                џџџџ    @        џџџџ    џџџџ         ;   L   D:\Projects\DFramework\test\test\shaders\simpleShader.hlsl Structures.hlsli Utilits.hlsli   ўяўя                  џџџџџџџџџџ џџџџџџџџџџ                                                                                                                                                    	    s  T       џ	  и       M   м  T      (   8  ,   T                                           	   
                                                                                                                                                                                                                                                                                                                                                                              !                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               