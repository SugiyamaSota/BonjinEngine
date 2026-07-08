
struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t3 texcoord : TEXCOORD0;
};

struct TransformationMatrix
{
    matrix WVP;
};

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    int32_t enableSpecular;
    float32_t4x4 uvTransform;
    float32_t shininess;
};
