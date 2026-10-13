// GpuLightning.hlsli - 雷GPUパーティクル共通ヘッダー

struct ParticleDataGPU
{
    float32_t3 position;       // 座標
    float32_t  scale;          // サイズ
    float32_t4 color;          // 色 (RGB) + アルファ (A)
    float32_t  intensity;      // 発光強度 (Bloom/加算発光用)
    float32_t  lifeRatio;      // 生存割合 (0.0=誕生, 1.0=寿命終了)
    uint32_t   particleType;   // 0: 雷ボルトの節点/セグメント, 1: 飛散スパーク, 2: 接地点グロー
    float32_t  customParam;    // 予備/角度/太さパラメータ
};

struct SceneBuffer
{
    float32_t4x4 viewProjection;
    float32_t4x4 view;
    float32_t4x4 billboardMatrix;
    float32_t3   cameraPosition;
    float32_t    time;
};

struct VertexInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t4 color    : COLOR0;
};

struct VertexOutput
{
    float32_t4 position     : SV_POSITION;
    float32_t2 texcoord     : TEXCOORD0;
    float32_t4 color        : COLOR0;
    float32_t  intensity    : INTENSITY0;
    uint32_t   particleType : TYPE0;
};
