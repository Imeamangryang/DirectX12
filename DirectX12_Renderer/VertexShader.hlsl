Texture2D<float4> heightmap : register(t0);

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 norm : NORMAL;
	float2 tex : TEXCOORD;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 viewproj;
	float4 eye;
	int height;
	int width;
}

VS_OUTPUT VS(float3 input : POSITION) {
	VS_OUTPUT output;

	output.pos = float4(input.x, input.y, input.z, 1.0f);
	output.pos = mul(output.pos, viewproj);

	output.norm = float4(normalize(float3(input)), 1.0f);

	float theta = atan2(output.norm.z, output.norm.x);
	float phi = acos(output.norm.y);

	output.tex.x = theta / (2.0f * 3.14159265359f);
	output.tex.y = phi / 3.14159265359f;

	return output;
}
