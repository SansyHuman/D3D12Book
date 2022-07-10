#include "LightingUtils.hlsl"

Texture2D gDiffuseMap : register(t0);

SamplerState gsamLinearWrap : register(s0);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gWorldInvTranspose;
    float4x4 gTexTransform;
};

cbuffer cbMaterial : register(b1)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

cbuffer cbPass : register(b2)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float3 gEyePosW;
	float gGamma;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
	float gUnscaledTotalTime;
	float gUnscaledDeltaTime;
	float gFogStart;
    float gFogRange;
    float4 gFogColor;
    float4 gAmbientLight;
    Light gLights[MaxLights];
    int gNumLights;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_Position;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;

    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    vout.NormalW = mul(vin.NormalL, (float3x3)gWorldInvTranspose);

    vout.PosH = mul(posW, gViewProj);

    vout.TexC = mul(mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform), gMatTransform).xy;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinearWrap, pin.TexC) * gDiffuseAlbedo;

    #ifdef ALPHA_TEST
    clip(diffuseAlbedo.a - 0.1f);
    #endif

    pin.NormalW = normalize(pin.NormalW);
    float3 toEyeW = gEyePosW - pin.PosW;
    float distToEye = length(toEyeW);
    toEyeW /= distToEye;

    float4 ambient = gAmbientLight * diffuseAlbedo;

    const float shininess = 1.0f - gRoughness;
    Material mat = {diffuseAlbedo, gFresnelR0, shininess};
    float shadowFactor[MaxLights];
    for(int i = 0; i < MaxLights; i++)
    {
        shadowFactor[i] = 1.0f;
    }
    float4 directLight = ComputeLighting(gLights, gNumLights, mat, pin.PosW, pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;
    
    #ifdef FOG
    float fogAmount = saturate(pow(max(0.0f, (distToEye - gFogStart) / gFogRange), 1.5f));
    litColor = lerp(litColor, gFogColor, fogAmount);
    #endif

    litColor.rgb = pow(litColor.rgb, float3(1.0f / gGamma, 1.0f / gGamma, 1.0f / gGamma));
    litColor.a = diffuseAlbedo.a;

    return litColor;
}
