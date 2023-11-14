struct VS_OUTPUT
{
	float4 pos : POSITION0;
	float4 norm : NORMAL;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 viewproj;
	float4 eye;
	int height;
	int width;
}

Texture2D<float4> heightmap : register(t0);

VS_OUTPUT VSTes(float3 input : POSITION) {
	VS_OUTPUT output;

	float scale = height / 4;
	float4 mysample = heightmap.Load(int3(input));
	output.pos = float4(input.x, input.y, mysample.r * scale, 1.0f);

	float zb = heightmap.Load(int3(input.xy + int2(0, -1), 0)).r * scale;
	float zc = heightmap.Load(int3(input.xy + int2(1, 0), 0)).r * scale;
	float zd = heightmap.Load(int3(input.xy + int2(1, 1), 0)).r * scale;
	float ze = heightmap.Load(int3(input.xy + int2(0, 1), 0)).r * scale;
	float zf = heightmap.Load(int3(input.xy + int2(-1, 0), 0)).r * scale;
	float zg = heightmap.Load(int3(input.xy + int2(-1, -1), 0)).r * scale;

	float x = 2 * zf + zc + zg - zb - 2 * zc - zd;
	float y = 2 * zb + zc + zg - zd - 2 * ze - zf;
	float z = 6.0f;

	output.norm = float4(normalize(float3(x, y, z)), 1.0f);

	return output;
}