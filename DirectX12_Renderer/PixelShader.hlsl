Texture2D<float4> heightmap : register(t0);
Texture2D<float4> colormap : register(t1);
SamplerState hmsampler : register(s0);
SamplerState cmsampler : register(s1);

cbuffer ConstantBuffer : register(b0)
{
	float4x4 viewproj;
	float4 eye;
	int height;
	int width;
}

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 norm : NORMAL;
	float2 tex : TEXCOORD;
};

float4 PS(VS_OUTPUT input) : SV_TARGET
{
	return colormap.Sample(cmsampler, input.tex);
}