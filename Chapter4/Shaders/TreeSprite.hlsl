#include "LightingUtils.hlsl"

Texture2DArray gTreeMapArray : register(t0);

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
	float3 PosW : POSITION;
	float2 SizeW : SIZE;
};

struct VertexOut
{
	float3 CenterW : POSITION;
	float2 SizeW : SIZE;
};

struct GeoOut
{
	float4 PosH : SV_Position;
	float3 PosW : POSITION;
	float3 NormalW : NORMAL;
	float2 TexC : TEXCOORD;
	uint PrimID : SV_PrimitiveID;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.CenterW = vin.PosW;
	vout.SizeW = vin.SizeW;

	return vout;
}

[maxvertexcount(4)]
void GS(
	point VertexOut gin[1],
	uint primID : SV_PrimitiveID, 
	inout TriangleStream<GeoOut> triStream)
{
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look = gEyePosW - gin[0].CenterW;
	look.y = 0.0f;
	look = normalize(look);
	float3 right = cross(up, look);

	float halfWidth = 0.5f * gin[0].SizeW.x;
	float halfHeight = 0.5f * gin[0].SizeW.y;

	float4 v[4];
	v[0] = float4(gin[0].CenterW + halfWidth * right - halfHeight * up, 1.0f);
	v[1] = float4(gin[0].CenterW + halfWidth * right + halfHeight * up, 1.0f);
	v[2] = float4(gin[0].CenterW - halfWidth * right - halfHeight * up, 1.0f);
	v[3] = float4(gin[0].CenterW - halfWidth * right + halfHeight * up, 1.0f);

	float2 texC[4] = {
		float2(0.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(1.0f, 1.0f),
		float2(1.0f, 0.0f)
	};

	GeoOut gout;

	[unroll(4)]
	for(int i = 0; i < 4; i++)
	{
		gout.PosH = mul(v[i], gViewProj);
		gout.PosW = v[i].xyz;
		gout.NormalW = look;
		gout.TexC = texC[i];
		gout.PrimID = primID;

		triStream.Append(gout);
	}
}

float4 PS(GeoOut pin) : SV_Target
{
	float3 uvw = float3(pin.TexC, pin.PrimID % 3);
	float4 diffuseAlbedo = gTreeMapArray.Sample(gsamLinearWrap, uvw) * gDiffuseAlbedo;

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