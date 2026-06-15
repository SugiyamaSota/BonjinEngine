#include "FullScreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float32_t4x4 projectionInverse;
    int32_t isGray;
    int32_t isVignette;
    float32_t2 padding;
};
ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutPut
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutPut main(VertexShaderOutPut input)
{
    PixelShaderOutPut output;
    output.color = gTexture.Sample(gSampler, input.texcoord);
    
    if (gMaterial.isGray != 0)
    {
        float32_t value = dot(output.color.rgb, float32_t3(0.2125f, 0.7154f, 0.0721f));
        output.color.rgb = float32_t3(value, value, value);
    }
    
    if(gMaterial.isVignette != 0)
    {
    // 中心になるほど明るくなるようにする
        float32_t2 correct = input.texcoord * (1.f - input.texcoord.yx);
    // scaleで調整
        float vignette = correct.x * correct.y * 16.f;
    // とりあえず0.8乗
        vignette = saturate(pow(vignette, 0.8f));
    // 出力に乗算
        output.color.rgb *= vignette;
    }
    
    return output;
}
