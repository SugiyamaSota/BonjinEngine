#include "GpuLightning.hlsli"

ConstantBuffer<SceneBuffer> gScene : register(b0);
StructuredBuffer<ParticleDataGPU> gParticles : register(t0);

VertexOutput main(VertexInput input, uint32_t instanceId : SV_InstanceID)
{
    VertexOutput output;
    ParticleDataGPU particle = gParticles[instanceId];

    // 生存期間が終了している（lifeRatio >= 1.0 または scale <= 0）場合は縮退させて描画をスキップ
    if (particle.lifeRatio >= 1.0f || particle.scale <= 0.0f)
    {
        output.position = float32_t4(0.0f, 0.0f, -1000.0f, 1.0f);
        output.texcoord = float32_t2(0.0f, 0.0f);
        output.color = float32_t4(0.0f, 0.0f, 0.0f, 0.0f);
        output.intensity = 0.0f;
        output.particleType = 0;
        return output;
    }

    // ビルボード変形: 頂点のローカル座標をカメラ向きに回転＆スケール適用
    float32_t3 localPos = input.position.xyz * particle.scale;
    float32_t3 worldVertexOffset = mul(float32_t4(localPos, 0.0f), gScene.billboardMatrix).xyz;

    // ワールド座標
    float32_t3 worldPos = particle.position + worldVertexOffset;

    // 射影変換 (WVP)
    output.position = mul(float32_t4(worldPos, 1.0f), gScene.viewProjection);
    output.texcoord = input.texcoord;
    
    // アルファの指数減衰（急峻なフェード）
    float32_t alpha = particle.color.a * saturate(1.0f - particle.lifeRatio * particle.lifeRatio);
    output.color = float32_t4(particle.color.rgb, alpha);
    output.intensity = particle.intensity;
    output.particleType = particle.particleType;

    return output;
}
