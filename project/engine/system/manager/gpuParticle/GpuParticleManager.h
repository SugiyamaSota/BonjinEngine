#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <random>
#include "GpuParticleStructs.h"

class Camera;
class DirectXCommon;

namespace Bonjin {

class GpuParticleManager {
public:
    static GpuParticleManager* GetInstance();

    void Initialize();
    void Finalize();

    void Update(float deltaTime, const Camera* camera);
    void Draw(const Camera* camera);

    void EmitLightning(const GpuLightningConfig& config);
    void EmitSparks(const Vector3& pos, int count, const Vector4& color, float speed = 5.0f, float lifeTime = 0.4f, float scale = 0.12f);
    void Clear();
    void DrawImGui();

private:
    GpuParticleManager() = default;
    ~GpuParticleManager() = default;
    GpuParticleManager(const GpuParticleManager&) = delete;
    GpuParticleManager& operator=(const GpuParticleManager&) = delete;

    void CreateBuffers();
    int AllocateParticle();
    void GenerateSubdividedLine(const Vector3& start, const Vector3& end, float maxOffset, int depth, std::vector<Vector3>& outPoints);

private:
    static constexpr uint32_t kMaxParticles = 16384;
    static constexpr uint32_t kVertexCount = 4;
    static constexpr uint32_t kIndexCount = 6;

    DirectXCommon* dxCommon_ = nullptr;
    ID3D12Device* device_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> sceneResource_;
    GpuSceneBuffer* sceneData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
    ParticleDataGPU* mappedGpuData_ = nullptr;
    uint32_t srvIndex_ = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_{};

    std::vector<ParticleState> particles_;
    uint32_t activeCount_ = 0;
    float globalTime_ = 0.0f;

    std::mt19937 randomEngine_;
    std::uniform_real_distribution<float> dist01_{ 0.0f, 1.0f };
    std::uniform_real_distribution<float> distSigned_{ -1.0f, 1.0f };

    GpuLightningConfig testConfig_{};
};

} // namespace Bonjin
