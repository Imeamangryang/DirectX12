Texture2D<float4> displacementmap : register(t0);
SamplerState dmsampler : register(s0);

struct VS_OUTPUT
{
	float4 pos : POSITION0;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 viewproj;
	float4 eye;
	int height;
	int width;
}

VS_OUTPUT VSTes(float3 input : POSITION) {
	VS_OUTPUT output;

	output.pos = float4(input.x, input.y, input.z, 1.0f);

	return output;
}