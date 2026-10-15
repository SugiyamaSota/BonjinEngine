#include "FullScreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutPut
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutPut main(VertexShaderOutPut input)
{
    PixelShaderOutPut output;

    // フェード率（0.0: 完全な真っ黒 ～ 1.0: 通常画面）
    float fade = saturate(gMaterial.dissolveThreshold);

    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = float4(texColor.rgb * fade, 1.0f);

    return output;
}
