Texture2D<float4> heightmap : register(t0);

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 worldpos : POSITION;
	float4 norm : NORMAL;
	//float4 tex : TEXCOORD;
	float2 tex : TEXCOORD;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 viewproj;
	float4 eye;
	int height;
	int width;
}

//VS_OUTPUT VS(float3 input : POSITION) {
//	VS_OUTPUT output;
//
//	float scale = height / 100;
//	float4 mysample = heightmap.Load(int3(input));
//	output.pos = float4(input.x, input.y, input.z, 1.0f);
//	output.tex = float4(input.x / width, input.y / height, output.pos.z, scale);
//	output.pos = mul(output.pos, viewproj);
//
//	float zb = heightmap.Load(int3(input.xy + int2(0, -1), 0)).r * scale;
//	float zc = heightmap.Load(int3(input.xy + int2(1, 0), 0)).r * scale;
//	float zd = heightmap.Load(int3(input.xy + int2(1, 1), 0)).r * scale;
//	float ze = heightmap.Load(int3(input.xy + int2(0, 1), 0)).r * scale;
//	float zf = heightmap.Load(int3(input.xy + int2(-1, 0), 0)).r * scale;
//	float zg = heightmap.Load(int3(input.xy + int2(-1, -1), 0)).r * scale;
//
//	float x = 2 * zf + zc + zg - zb - 2 * zc - zd;
//	float y = 2 * zb + zc + zg - zd - 2 * ze - zf;
//	float z = 6.0f;
//
//	output.norm = float4(normalize(float3(x, y, z)), 1.0f);
//
//	output.worldpos = float4(input, 1.0f);
//
//	return output;
//}

VS_OUTPUT VS(float3 input : POSITION) {
	VS_OUTPUT output;

	output.pos = float4(input.x, input.y, input.z, 1.0f);
	output.pos = mul(output.pos, viewproj);

	output.norm = float4(normalize(float3(input)), 1.0f);

	float theta = atan2(output.norm.z, output.norm.x);
	float phi = acos(output.norm.y);

	output.tex.x = theta / (2.0f * 3.14159265359f);
	output.tex.y = phi / 3.14159265359f;

	output.worldpos = float4(input, 1.0f);

	return output;
}
