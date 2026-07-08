
struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
    float32_t3 worldPosition : POSITION0;
};

struct DirectionalLight
{
    float32_t4 color;
    float32_t3 direction;
    float32_t intensity;
};

struct PointLight
{
    float32_t4 color;
    float32_t3 position;
    float32_t intensity;
    float32_t radius;
    float32_t decay;
};

// スポット
struct SpotLight
{
    float32_t4 color; // 色
    float32_t3 position; // 位置
    float32_t intensity; // 輝度
    float32_t3 direction; // 方向
    float32_t distance; // ライトが届く最大距離
    float32_t decay; // 減衰率
    float32_t cosAngle; // スポットライトの余弦
};


struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    int32_t enableSpecular;
    float32_t4x4 uvTransform;
    float32_t shininess;
    uint32_t enableEnvironmentMap;
    float32_t environmentCoefficient;
};

struct Camera
{
    float32_t3 worldPosition;
};