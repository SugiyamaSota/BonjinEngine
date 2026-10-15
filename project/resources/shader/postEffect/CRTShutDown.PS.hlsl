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
    float progress = saturate(gMaterial.crtShutDownProgress);

    // 進行度 0.0 は無加工で出力
    if (progress <= 0.0001f)
    {
        output.color = gTexture.Sample(gSampler, input.texcoord);
        return output;
    }

    // 進行度 0.97 以上は完全な漆黒（ブラックアウト）
    if (progress >= 0.97f)
    {
        output.color = float32_t4(0.0f, 0.0f, 0.0f, 1.0f);
        return output;
    }

    float2 uv = input.texcoord;
    float2 centered = uv - 0.5f;

    // ========================================================
    // 1. 垂直方向の急激な押し潰し（画面の映像が上下から中央へ圧縮される）
    // progress: 0.02 ~ 0.55
    // ========================================================
    float vCollapse = smoothstep(0.02f, 0.55f, progress);
    float vScale = max(0.0012f, pow(1.0f - vCollapse, 2.6f));
    centered.y /= vScale;

    // ========================================================
    // 2. 水平方向の収束（左右から中央の一点へシュッと縮む）
    // progress: 0.55 ~ 0.92
    // ========================================================
    float hScale = 1.0f;
    if (progress > 0.55f)
    {
        float hCollapse = smoothstep(0.55f, 0.92f, progress);
        hScale = max(0.001f, pow(1.0f - hCollapse, 2.4f));
        centered.x /= hScale;
    }

    float2 sampledUV = centered + 0.5f;

    // 圧縮された領域の外側（ブラウン管の消灯領域）は完全な黒
    if (sampledUV.x < 0.0f || sampledUV.x > 1.0f || sampledUV.y < 0.0f || sampledUV.y > 1.0f)
    {
        // わずかに境界にグローを持たせる
        float lineDist = abs(input.texcoord.y - 0.5f);
        float borderGlow = exp(-lineDist * 500.0f) * 1.5f * (1.0f - vScale);
        if (progress > 0.55f)
        {
            borderGlow *= hScale;
        }
        output.color = float32_t4(float3(0.7f, 0.9f, 1.0f) * borderGlow, 1.0f);
        return output;
    }

    // 元の映像をサンプリング
    float4 srcColor = gTexture.Sample(gSampler, sampledUV);

    // ========================================================
    // 3. 過電圧エネルギー集中による発光・ホワイトアウト（CRTフラッシュ）
    // ========================================================
    // 潰れれば潰れるほど、エネルギー密度が上がって映像が白く眩しく輝く！
    float energyBoost = (1.0f - vScale) * 3.8f;
    float3 emission = float3(1.1f, 1.35f, 1.7f) * energyBoost;

    // 瞬間閃光スパーク (0.00 ~ 0.10)
    if (progress < 0.10f)
    {
        float spark = (1.0f - (progress / 0.10f)) * 1.2f;
        emission += float3(spark, spark, spark);
    }

    // 中央ライン・ドットの強烈な発光グロー
    float distToCenterY = abs(input.texcoord.y - 0.5f);
    float lineGlow = exp(-distToCenterY * 400.0f) * 3.5f * (1.0f - vScale);

    float dotGlow = 0.0f;
    if (progress > 0.60f)
    {
        float d = length(input.texcoord - 0.5f);
        dotGlow = exp(-d * 150.0f) * 5.0f;
    }

    float3 finalRgb = srcColor.rgb * (1.0f + energyBoost * 0.6f) + emission * 0.35f + float3(0.9f, 0.95f, 1.0f) * (lineGlow + dotGlow);

    // 残光の「プチッ」消滅 (0.92 ~ 0.97)
    if (progress > 0.92f)
    {
        float fade = 1.0f - smoothstep(0.92f, 0.965f, progress);
        finalRgb *= fade;
    }

    output.color = float32_t4(finalRgb, 1.0f);
    return output;
}
