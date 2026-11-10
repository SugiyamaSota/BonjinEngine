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

    // 光沢リング: 細い輪 + 回り込むハイライトでガラス/メタルのような質感
    if (input.particleType == 3)
    {
        const float32_t ringRadius = 0.78f;
        const float32_t ringWidth = 0.075f;
        float32_t d = (dist - ringRadius) / ringWidth;

        float32_t band = exp(-d * d);                       // 輪の本体
        float32_t edge = exp(-d * d * 9.0f);                // 中央の鋭い白ライン
        float32_t halo = exp(-d * d * 0.12f) * 0.25f;       // 外側の柔らかい滲み
        float32_t innerD = (dist - ringRadius * 0.86f) / (ringWidth * 0.4f);
        float32_t innerSheen = exp(-innerD * innerD) * 0.35f; // 内側の映り込みライン

        // 回転するスペキュラー（customParam = 光源角度[rad]）
        float32_t2 dir = (dist > 0.0001f) ? normalize(centerOffset) : float32_t2(0.0f, -1.0f);
        float32_t2 lightDir = float32_t2(cos(input.customParam), sin(input.customParam));
        float32_t facing = dot(dir, lightDir);
        float32_t spec = pow(saturate(facing), 14.0f) * 1.6f;     // メインハイライト
        spec += pow(saturate(-facing), 30.0f) * 0.6f;             // 反対側の弱い反射
        float32_t sparkle = pow(saturate(facing), 80.0f) * 2.5f;  // 鋭いきらめき点

        float32_t3 white = float32_t3(1.0f, 1.0f, 1.0f);
        float32_t3 ringRgb =
            input.color.rgb * (band * 0.9f + halo) +
            white * (edge * 0.55f + band * spec + edge * sparkle + innerSheen * (0.4f + spec));
        float32_t ringAlpha = saturate(band + halo + innerSheen + band * spec) * input.color.a;

        output.color = float32_t4(ringRgb * input.intensity, ringAlpha);
        return output;
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

    // 芯 (Core) + 外周オーラ (Glow)
    float32_t3 finalRgb = (input.color.rgb * core * coreWeight + input.color.rgb * aura) * input.intensity;
    float32_t finalAlpha = (core + aura * 0.8f) * input.color.a;

    output.color = float32_t4(finalRgb, saturate(finalAlpha));
    return output;
}
