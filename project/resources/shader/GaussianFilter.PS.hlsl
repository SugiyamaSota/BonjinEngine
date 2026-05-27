#include "FullScreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

static const float32_t2 kIndex3x3[3][3] =
{
    { { -1.f, -1.f }, { 0.f, -1.f }, { 1.f, -1.f } },
    { { -1.f, 0.f }, { 0.f, 0.f }, { 1.f, 0.f } },
    { { -1.f, 1.f }, { 0.f, 1.f }, { 1.f, 1.f } },
};

static const float32_t PI = 3.14159265f;

float gauss(float x, float y, float sigma)
{
    float exponent = -(x * x + y * y) * rcp(2.f * sigma * sigma);
    float denominator = 2.f * PI * sigma * sigma;
    return exp(exponent) * rcp(denominator);
}

struct PixelShaderOutPut
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutPut main(VertexShaderOutPut input)
{
    
    float32_t weight = 0.f;
    float32_t kernel3x3[3][3];
    for (int32_t x = 0; x < 3; ++x)
    {
        for (int32_t y = 0; y < 3; ++y)
        {
            kernel3x3[x][y] = gauss(kIndex3x3[x][y].x, kIndex3x3[x][y].y, 2.f);
            weight += kernel3x3[x][y];
        }
    }
    
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp(width), rcp(height));
    
    PixelShaderOutPut output;
    
    output.color.rgb = float32_t3(0.f, 0.f, 0.f);
    output.color.a = 1.f;
    
    for (int32_t x = 0; x < 3; ++x)
    {
        for (int32_t y = 0; y < 3; ++y)
        {
            float32_t2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;
            
            float32_t3 fetchColor = gTexture.Sample(gSampler, texcoord).rgb;
            output.color.rgb += fetchColor * kernel3x3[x][y];
        }
    }
    
    output.color.rgb *= rcp(weight);
    
    return output;
}