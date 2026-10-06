#include "GpuLightning.hlsli"

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexOutput input)
{
    PixelShaderOutput output;

    // UV中心 (0.5, 0.5) からの距離 [0.0 - 1.0]
    float32_t2 centerOffset = input.texcoord - float32_t2(0.5f, 0.5f);
    float32_t dist = length(centerOffset) * 2.0f;

    if (dist > 1.0f)
    {
        discard;
    }

    // 1. スパーク・雷コアの鋭い光 (中心の白光)
    float32_t core = exp(-dist * 5.0f);

    // 2. 外周の柔らかな放電オーラ (電撃色のグロー)
    float32_t aura = saturate(1.0f - dist);
    aura = pow(aura, 2.2f);

    // 3. タイプ別の微調整
    float32_t coreWeight = 0.8f;
    if (input.particleType == 0) // 雷ボルトの節点: コアをより強く
    {
        coreWeight = 1.2f;
    }
    else if (input.particleType == 1) // スパーク火花: 粒子感を強調
    {
        coreWeight = 0.5f;
    }

    // 白い芯 (Core) + 指定色 (Glow)
    float32_t3 finalRgb = (float32_t3(1.0f, 1.0f, 1.0f) * core * coreWeight + input.color.rgb * aura) * input.intensity;
    float32_t finalAlpha = (core + aura * 0.8f) * input.color.a;

    output.color = float32_t4(finalRgb, saturate(finalAlpha));
    return output;
}
