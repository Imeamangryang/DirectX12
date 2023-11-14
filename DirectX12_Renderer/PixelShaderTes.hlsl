Texture2D<float4> heightmap : register(t0);
SamplerState hmsampler : register(s0);

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
};

float4 PSTes(VS_OUTPUT input) : SV_TARGET
{
	float4 light = normalize(float4(1.0f, 1.0f, -1.0f, 1.0f));
	float diffuse = saturate(dot(input.norm, -light));
	float ambient = 0.2f;
	float3 color = float3(1.0f, 1.0f, 1.0f);

	return float4(saturate((color * diffuse) + (color * ambient)), 1.0f);
}