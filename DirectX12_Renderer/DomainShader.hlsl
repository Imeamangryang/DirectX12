Texture2D<float4> displacementmap : register(t0);
SamplerState dmsampler : register(s0);

cbuffer ConstantBuffer : register(b0)
{
	float4x4 viewproj;
	float4 eye;
	int height;
	int width;
}

struct DS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 norm : NORMAL;
	float2 tex : TEXCOORD;
};

struct HS_CONTROL_POINT_OUTPUT
{
	float4 pos : POSITION0;
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]	: SV_TessFactor; 
	float InsideTessFactor : SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 3

[domain("tri")]
DS_OUTPUT DS(
	HS_CONSTANT_DATA_OUTPUT input,
	float3 domain : SV_DomainLocation,
	const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
	DS_OUTPUT output;

	output.pos = float4(patch[0].pos.xyz * domain.x + patch[1].pos.xyz * domain.y + patch[2].pos.xyz * domain.z, 1);
	
	output.norm = float4(normalize(float3(output.pos.xyz)), 1.0f);

	//float scale = height / 10;
	float scale = height / 100;

	float theta = atan2(output.norm.z, output.norm.x);
	float phi = acos(output.norm.y);

	output.tex.x = theta / (2.0f * 3.14159265359f);
	output.tex.y = phi / 3.14159265359f;

	float h = displacementmap.SampleLevel(dmsampler, output.tex.xy, 0).r;

	output.pos.xyz += output.norm * (scale * h);

	output.pos = float4(output.pos.xyz, 1.0f);

	output.pos = mul(output.pos, viewproj);

	return output;
}

