Texture2D<float4> colormap : register(t1);
SamplerState cmsampler : register(s1);

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

float4 PSTes(DS_OUTPUT input) : SV_TARGET
{
	return colormap.Sample(cmsampler, input.tex);
}