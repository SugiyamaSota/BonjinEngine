#include "FullScreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t4> gMaskTexture : register(t1);
SamplerState gSampler : register(s0);

struct PixelShaderOutPut
{
    float32_t4 color : SV_TARGET0;
};

// 疑似乱数ハッシュ（ブロックごとに一意かつランダムな値を生成）
float Hash21(float2 p)
{
    p = frac(p * float2(123.34f, 456.21f));
    p += dot(p, p + 45.32f);
    return frac(p.x * p.y);
}

PixelShaderOutPut main(VertexShaderOutPut input)
{
    PixelShaderOutPut output;

    // 進行度を外部から取得
    float progress = saturate(gMaterial.dissolveThreshold);

    // 0.0なら完全な真っ黒
    if (progress <= 0.0001f)
    {
        output.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return output;
    }
    // 1.0なら通常のゲーム画面
    if (progress >= 0.9999f)
    {
        output.color = gTexture.Sample(gSampler, input.texcoord);
        return output;
    }

    // デジタルブロック分割
    float2 blockSize = float2(128.0f, 72.0f);
    float2 blockCoord = floor(input.texcoord * blockSize);
    float2 blockUV = blockCoord / blockSize;

    // ブロックごとのランダム値
    float blockRand = Hash21(blockCoord);
    
    // サブブロックのノイズ
    float subRand = Hash21(floor(input.texcoord * blockSize * 2.0f));

    // 時間によるちらつき
    float timeStep = floor(gMaterial.time * 24.0f);
    float timeFlicker = Hash21(blockCoord + float2(timeStep * 0.07f, timeStep * 0.13f));

    // スキャンの進行/モードをMaterialから取得して分岐
    // dissolveMode: 0 = 下から上(BottomToTop), 1 = 上から下(TopToBottom), 2 = 画面全体ランダム(Random), 3 = 左から右, 4 = 右から左
    float blockThreshold = 0.0f;
    if (gMaterial.dissolveMode == 1)
    {
        // 上から下へスキャン
        float scanPos = blockUV.y;
        blockThreshold = scanPos * 0.65f + blockRand * 0.25f + subRand * 0.10f;
    }
    else if (gMaterial.dissolveMode == 2)
    {
        // 画面全体からランダムにフェード（スキャン線なし）
        blockThreshold = blockRand * 0.80f + subRand * 0.20f;
    }
    else if (gMaterial.dissolveMode == 3)
    {
        // 左から右へスキャン
        float scanPos = blockUV.x;
        blockThreshold = scanPos * 0.65f + blockRand * 0.25f + subRand * 0.10f;
    }
    else if (gMaterial.dissolveMode == 4)
    {
        // 右から左へスキャン
        float scanPos = 1.0f - blockUV.x;
        blockThreshold = scanPos * 0.65f + blockRand * 0.25f + subRand * 0.10f;
    }
    else
    {
        // デフォルト: 下から上へスキャン
        float scanPos = 1.0f - blockUV.y;
        blockThreshold = scanPos * 0.65f + blockRand * 0.25f + subRand * 0.10f;
    }

    // 進行度との差分（正なら出現済み、負なら未出現）
    float edgeWidth = max(gMaterial.dissolveEdgeWidth, 0.02f);
    // progressを 0.0~1.0 から blockThreshold の全域をカバーするスケールへマッピング
    float normalizedProgress = progress * (1.0f + edgeWidth * 2.0f) - edgeWidth;
    float diff = normalizedProgress - blockThreshold;

    // 4. ブロック内のワイヤーフレーム枠線（グリッド線）の検出
    float2 cellUV = frac(input.texcoord * blockSize);
    float2 border = step(0.90f, cellUV) + step(cellUV, 0.10f);
    float gridBorder = max(border.x, border.y);

    // 水平スキャンライン（走査線）
    float scanline = sin(input.texcoord.y * 720.0f * 3.14159f) * 0.5f + 0.5f;

    // 5. 状態に応じたレンダリング
    float3 cyberColor = gMaterial.dissolveEdgeColor;

    if (diff < -edgeWidth)
    {
        // --- 【未出現エリア】完全な漆黒 ---
        // まれに走る微小な待機グリッドの点滅（データロードの予兆感）
        float standbyGrid = gridBorder * step(0.97f, blockRand) * step(0.7f, timeFlicker) * 0.25f;
        output.color = float4(cyberColor * standbyGrid, 1.0f);
    }
    else if (diff < 0.0f)
    {
        // --- 【出現直前エリア（実体化の予兆）】 ---
        // 黒い空間にサイバーグリッドが浮かび上がり、ブロックがピピピッと青白く点滅
        float preGlow = (diff + edgeWidth) / edgeWidth; // 0.0 -> 1.0
        float flicker = (timeFlicker > 0.4f ? 1.0f : 0.2f);
        float3 glowCol = cyberColor * (gridBorder * 0.8f + scanline * 0.3f + 0.1f) * preGlow * flicker;
        output.color = float4(glowCol, 1.0f);
    }
    else if (diff < edgeWidth)
    {
        // --- 【出現境界エリア（スキャンフロント・激しい実体化）】 ---
        float edgeFactor = 1.0f - (diff / edgeWidth); // 1.0 -> 0.0
        
        // 水平ラインの激しいグリッチ（RGB色収差 / スライスズレ）
        float glitchTrigger = step(0.5f, timeFlicker);
        float glitchOffset = (timeFlicker - 0.5f) * 0.04f * edgeFactor * glitchTrigger;

        float2 uvR = input.texcoord + float2(glitchOffset + 0.006f * edgeFactor, 0.0f);
        float2 uvG = input.texcoord + float2(glitchOffset, 0.0f);
        float2 uvB = input.texcoord + float2(glitchOffset - 0.006f * edgeFactor, 0.0f);

        float4 colR = gTexture.Sample(gSampler, uvR);
        float4 colG = gTexture.Sample(gSampler, uvG);
        float4 colB = gTexture.Sample(gSampler, uvB);
        float3 baseCol = float3(colR.r, colG.g, colB.b);

        // 超高輝度ネオンサイバーエミッシブ（HDRブルーム対象）
        float3 edgeEmission = cyberColor * (edgeFactor * 2.5f + gridBorder * 1.5f + scanline * 0.6f);

        output.color = float4(saturate(baseCol + edgeEmission), 1.0f);
    }
    else
    {
        // --- 【出現完了エリア（復元された通常画面）】 ---
        // 出現直後の余韻（わずかな走査線とグリッチが滑らかに減衰）
        float settle = saturate((diff - edgeWidth) / 0.15f); // 0.0(直後) -> 1.0(完全復元)
        
        if (settle < 1.0f)
        {
            float remainingEdge = 1.0f - settle;
            float microGlitch = (timeFlicker - 0.5f) * 0.005f * remainingEdge;
            float3 baseCol = gTexture.Sample(gSampler, input.texcoord + float2(microGlitch, 0.0f)).rgb;
            float3 subGlow = cyberColor * (gridBorder * 0.3f + scanline * 0.2f) * remainingEdge;
            output.color = float4(saturate(baseCol + subGlow), 1.0f);
        }
        else
        {
            output.color = gTexture.Sample(gSampler, input.texcoord);
        }
    }

    return output;
}
