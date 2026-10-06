#include "GpuLightning.hlsli"

RWStructuredBuffer<ParticleDataGPU> gParticlesUAV : register(u0);

struct CSConstants
{
    float32_t deltaTime;
    uint32_t  maxParticles;
    float32_t gravity;
    float32_t drag;
};

ConstantBuffer<CSConstants> gCSConstants : register(b0);

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint32_t index = dispatchThreadID.x;
    if (index >= gCSConstants.maxParticles)
    {
        return;
    }

    ParticleDataGPU particle = gParticlesUAV[index];

    // 非アクティブな粒子はスキップ
    if (particle.lifeRatio >= 1.0f)
    {
        return;
    }

    // 寿命の進行
    particle.lifeRatio += gCSConstants.deltaTime;

    // スパーク粒子の物理シミュレーション (type == 1)
    if (particle.particleType == 1)
    {
        particle.position.y -= gCSConstants.gravity * gCSConstants.deltaTime;
        particle.scale *= (1.0f - gCSConstants.drag * gCSConstants.deltaTime);
    }
    
    // 指数関数的な発光減衰
    particle.intensity = exp(-particle.lifeRatio * 4.0f) * 2.5f;

    gParticlesUAV[index] = particle;
}
