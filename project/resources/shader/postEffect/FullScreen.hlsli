struct VertexShaderOutPut
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
};

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