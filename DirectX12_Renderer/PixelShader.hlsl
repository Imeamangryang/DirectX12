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
	float4 worldpos : POSITION;
	float4 norm : NORMAL;
	float4 tex : TEXCOORD;
};

float4 PS(VS_OUTPUT input) : SV_TARGET
{
	//float4 light = normalize(float4(1.0f, 1.0f, -1.0f, 1.0f));
	//float diffuse = saturate(dot(input.norm, -light));
	//float ambient = 0.2f;
	//float3 color = float3(1.0f, 1.0f, 1.0f);

	//return float4(saturate((color * diffuse) + (color * ambient)), 1.0f);

    //// Sample the heightmap texture
    //float heightValue = heightmap.Sample(hmsampler, input.tex.xy).r;

    //// Map the height value to a [0, 1] range
    //float normalizedHeight = heightValue / 255.0f;

    //// Sample the colormap using the normalized height value
    //float4 colorMapValue = colormap.Sample(cmsampler, input.tex).r;

    //// Lighting calculations
    //float4 light = normalize(float4(1.0f, 1.0f, -1.0f, 1.0f));
    //float diffuse = saturate(dot(input.norm, -light));
    //float ambient = 0.2f;

    //// Combine lighting with the modified colormap
    //float3 finalColor = (colorMapValue * diffuse) +( colorMapValue * ambient);

    //return float4(saturate(finalColor), 1.0f);
	return colormap.Sample(cmsampler, input.tex);
}