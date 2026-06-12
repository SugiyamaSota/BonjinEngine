#include "FullScreen.hlsli"

// --- テクスチャ・サンプラーの登録 ---
Texture2D<float32_t4> gTexture : register(t0); // カラーテクスチャ
Texture2D<float32_t> gDepthTexture : register(t1); // ★追加：深度テクスチャ

SamplerState gSampler : register(s0); // 通常の線形サンプラー
SamplerState gSamplerPoint : register(s1); // ★追加：ポイントサンプラー（深度のボケ防止）

// --- 定数バッファ ---
struct Material
{
    float32_t4x4 projectionInverse; // ★追加：NDC空間からView空間へ変換するための逆行列
};
ConstantBuffer<Material> gMaterial : register(b0); // ★追加

static const float32_t2 kIndex3x3[3][3] =
{
    { { -1.f, -1.f }, { 0.f, -1.f }, { 1.f, -1.f } },
    { { -1.f, 0.f }, { 0.f, 0.f }, { 1.f, 0.f } },
    { { -1.f, 1.f }, { 0.f, 1.f }, { 1.f, 1.f } },
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
            float32_t ndcDepth = gDepthTexture.Sample(gSamplerPoint, input.texcoord);
            float32_t4 viewSpace = mul(float32_t4(0.f,0.f, ndcDepth, 1.f), gMaterial.projectionInverse);
            float32_t viewZ = rcp(viewSpace.w); // ★追加：深度値を線形化
            difference.x += viewZ * kPrewittHorizontalKernel3x3[x][y];
            difference.y += viewZ * kPrewittVerticalKernel3x3[x][y];
        }
    }
    
    float32_t weight = length(difference);
    weight = saturate(weight);
    
    PixelShaderOutPut output;
    output.color.rgb = (1.f - weight) * gTexture.Sample(gSampler, input.texcoord).rgb;
    output.color.a = 1.f;
    
    return output;
}