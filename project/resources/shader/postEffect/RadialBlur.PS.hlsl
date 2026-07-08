#include "FullScreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);



struct PixelShaderOutPut
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutPut main(VertexShaderOutPut input)
{
    static const int32_t kNumSamples = 10;

    float32_t2 direction = input.texcoord - gMaterial.radialBlurCenter;
    float32_t3 outputColor = float32_t3(0.0f, 0.0f, 0.0f);

    for (int32_t sampleIndex = 0; sampleIndex < kNumSamples; ++sampleIndex)
    {
        float32_t2 texcoord = input.texcoord + direction * gMaterial.radialBlurWidth * float32_t(sampleIndex);
        outputColor += gTexture.Sample(gSampler, texcoord).rgb;
    }

    PixelShaderOutPut output;
    output.color.rgb = outputColor * rcp(float32_t(kNumSamples));
    output.color.a = 1.0f;
    return output;
}
