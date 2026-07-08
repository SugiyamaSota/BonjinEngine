#include "FullScreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t4> gMaskTexture : register(t1);
SamplerState gSampler : register(s0);



struct PixelShaderOutPut
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutPut main(VertexShaderOutPut input)
{
    float32_t mask = gMaskTexture.Sample(gSampler, input.texcoord).r;
    
    if (mask <= gMaterial.dissolveThreshold)
    {
        discard;
    }
    
    float32_t edgeWidth = max(gMaterial.dissolveEdgeWidth, 0.0001f);
    float32_t edge = 1.0f - smoothstep(
        gMaterial.dissolveThreshold,
        gMaterial.dissolveThreshold + edgeWidth,
        mask);
    
    PixelShaderOutPut output;
    output.color = gTexture.Sample(gSampler, input.texcoord);
    output.color.rgb = saturate(output.color.rgb + edge * gMaterial.dissolveEdgeColor);
    return output;
}
