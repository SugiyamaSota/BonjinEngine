#include "Sprite.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);

Texture2D<float32_t4> gTexture : register(t0);

SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
	float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;
	float32_t3 transformedUV = mul(float32_t4(input.texcoord.x, input.texcoord.y, 0.0f, 1.0f), gMaterial.uvTransform).xyz;
	float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
	output.color = gMaterial.color * textureColor;
	return output;
}
