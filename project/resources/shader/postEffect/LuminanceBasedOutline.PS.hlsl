#include "FullScreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

static const float32_t2 kIndex3x3[3][3] =
{
    { { -1.f, -1.f }, { 0.f, -1.f }, { 1.f, -1.f } },
    { { -1.f, 0.f }, { 0.f, 0.f }, { 1.f, 0.f } },
    { { -1.f, 1.f }, { 0.f, 1.f }, { 1.f, 1.f } },
};

static const float32_t kKernel3x3[3][3] =
{
    { 1.f / 9.f, 1.f / 9.f, 1.f / 9.f },
    { 1.f / 9.f, 1.f / 9.f, 1.f / 9.f },
    { 1.f / 9.f, 1.f / 9.f, 1.f / 9.f },
};

static const float32_t kPrewittHorizontalKernel3x3[3][3] =
{
    { -1.f / 6.f, 0.f, 1.f / 6.f },
    { -1.f / 6.f, 0.f, 1.f / 6.f },
    { -1.f / 6.f, 0.f, 1.f / 6.f },
};

static const float32_t kPrewittVerticalKernel3x3[3][3] =
{
    { -1.f / 6.f, -1.f / 6.f, -1.f / 6.f },
    { 0.f, 0.f, 0.f },
    { 1.f / 6.f, 1.f / 6.f, 1.f / 6.f },
};

float32_t Luminance(float32_t3 v)
{
    return dot(v, float32_t3(0.2126f, 0.7152f, 0.0722f));
}

struct PixelShaderOutPut
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutPut main(VertexShaderOutPut input)
{
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp(width), rcp(height));
    
    float32_t2 difference = float32_t2(0.f, 0.f);
    
    for (int32_t x = 0; x < 3; ++x)
    {
        for (int32_t y = 0; y < 3; ++y)
        {
            float32_t2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;
            float32_t3 fetchColor = gTexture.Sample(gSampler, texcoord).rgb;
            float32_t luminance = Luminance(fetchColor);
            difference.x += luminance * kPrewittHorizontalKernel3x3[x][y];
            difference.y += luminance * kPrewittVerticalKernel3x3[x][y];
        }
    }
    
    float32_t weight = length(difference);
    weight = saturate(weight * 6.f);
    
    PixelShaderOutPut output;
    output.color.rgb = (1.f-weight)*gTexture.Sample(gSampler, input.texcoord).rgb;
    output.color.a = 1.f;
    
    return output;
}