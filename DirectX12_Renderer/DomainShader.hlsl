Texture2D<float4> displacementmap : register(t0);
SamplerState dmsampler : register(s0);

struct LightData {
	float4 pos;
	float4 amb;
	float4 dif;
	float4 spec;
	float3 att;
	float rng;
	float3 dir;
	float sexp;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 viewproj;
	float4 eye;
	LightData light;
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

	//float3 x1 = displacementmap.Load(int3(output.pos.xy + int2(1, 0), 0));
	//float3 x2 = displacementmap.Load(int3(output.pos.xy + int2(-1, 0), 0));
	//float3 y1 = displacementmap.Load(int3(output.pos.xy + int2(0, 1), 0));
	//float3 y2 = displacementmap.Load(int3(output.pos.xy + int2(0, -1), 0));

	//// h1 = x1 - x2
	//// h2 = y1 - y2

	//float x = -2 * (x1.z - x2.z);
	//float y = -2 * (y1.z - y2.z);
	//float z = 4;

	//float zb = displacementmap.Load(int3(output.pos.xy + int2(0, -1), 0)).r * scale;
	//float zc = displacementmap.Load(int3(output.pos.xy + int2(1, 0), 0)).r * scale;
	//float zd = displacementmap.Load(int3(output.pos.xy + int2(1, 1), 0)).r * scale;
	//float ze = displacementmap.Load(int3(output.pos.xy + int2(0, 1), 0)).r * scale;
	//float zf = displacementmap.Load(int3(output.pos.xy + int2(-1, 0), 0)).r * scale;
	//float zg = displacementmap.Load(int3(output.pos.xy + int2(-1, -1), 0)).r * scale;
	//
	//float x = 2 * zf + zc + zg - zb - 2 * zc - zd;
	//float y = 2 * zb + zc + zg - zd - 2 * ze - zf;
	//float z = 6.0f;

	//output.norm = float4(normalize(float3(x, y, z)), 1.0f);

	output.pos = mul(output.pos, viewproj);

	return output;
}

