#include "FullScreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);



struct PixelShaderOutPut
{
    float32_t4 color : SV_TARGET0;
};

float32_t rand2dTo1d(float32_t2 value)
{
    float32_t2 dotDir = float32_t2(12.9898f, 78.233f);
    return frac(sin(dot(value, dotDir)) * 43758.5453f);
}

PixelShaderOutPut main(VertexShaderOutPut input)
{
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    
    float32_t2 pixelCoord = floor(input.texcoord * float32_t2(width, height));
    float32_t seed = gMaterial.time + 1.0f;
    float32_t random = rand2dTo1d(pixelCoord * seed);
    
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    float32_t noise = lerp(1.0f, random, saturate(gMaterial.noiseAlpha));
    
    PixelShaderOutPut output;
    output.color = float32_t4(textureColor.rgb * noise, textureColor.a);
    return output;
}
