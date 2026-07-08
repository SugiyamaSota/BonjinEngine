#include "FullScreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float32_t4x4 projectionInverse;
    int32_t isGray;
    int32_t isVignette;
    float32_t2 padding;
    float32_t2 radialBlurCenter;
    float32_t radialBlurWidth;
    float32_t dissolveThreshold;
    float32_t3 dissolveEdgeColor;
    float32_t dissolveEdgeWidth;
    float32_t time;
    float32_t noiseAlpha;
    float32_t hsvHueShift;
    float32_t hsvSaturationMultiplier;
    float32_t hsvValueMultiplier;
    float32_t3 padding3;
};
ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutPut
{
    float32_t4 color : SV_TARGET0;
};

// RGB to HSV conversion
float32_t3 RGBtoHSV(float32_t3 rgb)
{
    float32_t4 K = float32_t4(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);
    float32_t4 p = lerp(float32_t4(rgb.bg, K.wz), float32_t4(rgb.gb, K.xy), step(rgb.b, rgb.g));
    float32_t4 q = lerp(float32_t4(p.xyw, rgb.r), float32_t4(rgb.r, p.yzx), step(p.x, rgb.r));

    float32_t d = q.x - min(q.w, q.y);
    float32_t e = 1.0e-10f;
    return float32_t3(abs(q.z + (q.w - q.y) / (6.0f * d + e)), d / (q.x + e), q.x);
}

// HSV to RGB conversion
float32_t3 HSVtoRGB(float32_t3 hsv)
{
    float32_t4 K = float32_t4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float32_t3 p = abs(frac(hsv.xxx + K.xyz) * 6.0f - K.www);
    return hsv.z * lerp(K.xxx, saturate(p - K.xxx), hsv.y);
}

PixelShaderOutPut main(VertexShaderOutPut input)
{
    PixelShaderOutPut output;
    float32_t4 texColor = gTexture.Sample(gSampler, input.texcoord);
    
    // Convert to HSV
    float32_t3 hsv = RGBtoHSV(texColor.rgb);
    
    // Adjust HSV
    hsv.x = frac(hsv.x + gMaterial.hsvHueShift);
    if (hsv.x < 0.0f)
    {
        hsv.x += 1.0f;
    }
    hsv.y = saturate(hsv.y * gMaterial.hsvSaturationMultiplier);
    hsv.z = saturate(hsv.z * gMaterial.hsvValueMultiplier);
    
    // Convert back to RGB
    output.color.rgb = HSVtoRGB(hsv);
    output.color.a = texColor.a;
    
    return output;
}
