#define NOMINMAX
#include "GpuParticleManager.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "PSOManager.h"
#include "SrvManager.h"
#include "function/function.h"
#include "Matrix.h"
#include "Vector.h"
#include "Convert.h"
#include <numbers>
#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "../../externals/imgui/imgui.h"
#endif

namespace Bonjin {

GpuParticleManager* GpuParticleManager::GetInstance() {
    static GpuParticleManager instance;
    return &instance;
}

void GpuParticleManager::Initialize() {
    dxCommon_ = DirectXCommon::GetInstance();
    device_ = dxCommon_->GetDevice();

    std::random_device seed;
    randomEngine_ = std::mt19937(seed());

    particles_.resize(kMaxParticles);
    for (auto& p : particles_) {
        p.isActive = false;
    }

    CreateBuffers();

    // デフォルトテスト設定
    testConfig_.start = { -4.0f, 6.0f, 0.0f };
    testConfig_.end = { 0.0f, 0.0f, 0.0f };
    testConfig_.coreColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    testConfig_.glowColor = { 0.25f, 0.65f, 1.0f, 1.0f };
    testConfig_.boltRadius = 0.4f;
    testConfig_.boltScale = 0.22f;
    testConfig_.subdivisions = 7;
    testConfig_.mainBranches = 3;
    testConfig_.sparksPerSegment = 2;
    testConfig_.impactSparks = 80;
    testConfig_.duration = 0.16f;
    testConfig_.intensity = 4.0f;
    testConfig_.sparkSpeed = 7.0f;
}

void GpuParticleManager::Finalize() {
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
        vertexResource_.Reset();
    }
    if (indexResource_) {
        indexResource_->Unmap(0, nullptr);
        indexResource_.Reset();
    }
    if (sceneResource_) {
        sceneResource_->Unmap(0, nullptr);
        sceneResource_.Reset();
    }
    if (instancingResource_) {
        instancingResource_->Unmap(0, nullptr);
        instancingResource_.Reset();
    }
    mappedGpuData_ = nullptr;
    sceneData_ = nullptr;
}

void GpuParticleManager::CreateBuffers() {
    // 1. ビルボード板ポリゴンの頂点バッファ (Quad)
    GpuParticleVertex vertices[kVertexCount] = {
        { { -0.5f, -0.5f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }, // 左下
        { { -0.5f,  0.5f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }, // 左上
        { {  0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }, // 右下
        { {  0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }, // 右上
    };

    vertexResource_ = CreateBufferResource(device_, sizeof(vertices));
    GpuParticleVertex* vData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
    std::memcpy(vData, vertices, sizeof(vertices));

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(vertices);
    vertexBufferView_.StrideInBytes = sizeof(GpuParticleVertex);

    // 2. インデックスバッファ
    uint32_t indices[kIndexCount] = { 0, 1, 2, 2, 1, 3 };
    indexResource_ = CreateBufferResource(device_, sizeof(indices));
    uint32_t* iData = nullptr;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
    std::memcpy(iData, indices, sizeof(indices));

    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = sizeof(indices);
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    // 3. シーン用定数バッファ (b0)
    sceneResource_ = CreateBufferResource(device_, sizeof(GpuSceneBuffer));
    sceneResource_->Map(0, nullptr, reinterpret_cast<void**>(&sceneData_));
    *sceneData_ = GpuSceneBuffer{};

    // 4. インスタンシング用 StructuredBuffer (t0)
    instancingResource_ = CreateBufferResource(device_, sizeof(ParticleDataGPU) * kMaxParticles);
    instancingResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedGpuData_));

    for (uint32_t i = 0; i < kMaxParticles; ++i) {
        mappedGpuData_[i].position = { 0.0f, 0.0f, 0.0f };
        mappedGpuData_[i].scale = 0.0f;
        mappedGpuData_[i].color = { 0.0f, 0.0f, 0.0f, 0.0f };
        mappedGpuData_[i].intensity = 0.0f;
        mappedGpuData_[i].lifeRatio = 1.0f;
        mappedGpuData_[i].particleType = 0;
        mappedGpuData_[i].customParam = 0.0f;
    }

    // SRVの生成
    srvIndex_ = SrvManager::GetInstance()->Allocate();
    SrvManager::GetInstance()->CreateSrv(
        srvIndex_,
        instancingResource_.Get(),
        SrvType::StructuredBuffer,
        kMaxParticles,
        sizeof(ParticleDataGPU)
    );
    srvHandleGPU_ = SrvManager::GetInstance()->GetGPUHandle(srvIndex_);
}

int GpuParticleManager::AllocateParticle() {
    for (uint32_t i = 0; i < kMaxParticles; ++i) {
        if (!particles_[i].isActive) {
            particles_[i].isActive = true;
            return static_cast<int>(i);
        }
    }
    float maxLife = -1.0f;
    int oldestIndex = 0;
    for (uint32_t i = 0; i < kMaxParticles; ++i) {
        if (particles_[i].currentLife > maxLife) {
            maxLife = particles_[i].currentLife;
            oldestIndex = static_cast<int>(i);
        }
    }
    particles_[oldestIndex].isActive = true;
    return oldestIndex;
}

void GpuParticleManager::GenerateSubdividedLine(
    const Vector3& start, const Vector3& end, float maxOffset, int depth, std::vector<Vector3>& outPoints)
{
    if (depth <= 0) {
        outPoints.push_back(end);
        return;
    }

    Vector3 dir = Subtract(end, start);
    float length = Length(dir);
    if (length < 0.001f) {
        outPoints.push_back(end);
        return;
    }

    Vector3 dirNorm = Normalize(dir);
    Vector3 right = { -dirNorm.y, dirNorm.x, 0.0f };
    if (Length(right) < 0.001f) {
        right = { 0.0f, 1.0f, 0.0f };
    } else {
        right = Normalize(right);
    }
    Vector3 up = Cross(dirNorm, right);
    if (Length(up) > 0.001f) {
        up = Normalize(up);
    }

    Vector3 mid = Multiply(0.5f, Add(start, end));
    float offsetRight = distSigned_(randomEngine_) * maxOffset;
    float offsetUp = distSigned_(randomEngine_) * maxOffset * 0.8f;
    Vector3 displacedMid = Add(mid, Add(Multiply(offsetRight, right), Multiply(offsetUp, up)));

    GenerateSubdividedLine(start, displacedMid, maxOffset * 0.55f, depth - 1, outPoints);
    GenerateSubdividedLine(displacedMid, end, maxOffset * 0.55f, depth - 1, outPoints);
}

void GpuParticleManager::EmitLightning(const GpuLightningConfig& config) {
    std::vector<Vector3> mainPoints;
    mainPoints.push_back(config.start);
    GenerateSubdividedLine(config.start, config.end, config.boltRadius, config.subdivisions, mainPoints);

    // 1. メインボルトの節点パーティクル生成
    for (size_t i = 0; i < mainPoints.size(); ++i) {
        int idx = AllocateParticle();
        if (idx < 0) break;

        auto& p = particles_[idx];
        p.position = mainPoints[i];
        p.velocity = { 0.0f, 0.0f, 0.0f };
        p.color = config.coreColor;
        p.scale = config.boltScale;
        p.baseScale = config.boltScale;
        p.lifeTime = config.duration * (0.8f + 0.4f * dist01_(randomEngine_));
        p.currentLife = 0.0f;
        p.intensity = config.intensity;
        p.type = 0; // 雷ボルト

        // 節点から散るスパーク火花 (放電粒子)
        for (int s = 0; s < config.sparksPerSegment; ++s) {
            int sparkIdx = AllocateParticle();
            if (sparkIdx < 0) break;

            auto& sp = particles_[sparkIdx];
            sp.position = mainPoints[i];
            
            Vector3 randomDir = {
                distSigned_(randomEngine_),
                distSigned_(randomEngine_),
                distSigned_(randomEngine_)
            };
            if (Length(randomDir) > 0.001f) {
                randomDir = Normalize(randomDir);
            }
            float spSpeed = config.sparkSpeed * (0.5f + 0.5f * dist01_(randomEngine_));
            sp.velocity = Multiply(spSpeed, randomDir);

            sp.color = config.glowColor;
            float sparkScale = config.boltScale * 0.45f;
            sp.scale = sparkScale;
            sp.baseScale = sparkScale;
            sp.lifeTime = config.duration * (1.5f + 1.0f * dist01_(randomEngine_));
            sp.currentLife = 0.0f;
            sp.intensity = config.intensity * 0.8f;
            sp.type = 1; // スパーク
        }
    }

    // 2. 枝分かれ（ブランチ）の生成
    if (!mainPoints.empty() && config.mainBranches > 0) {
        for (int b = 0; b < config.mainBranches; ++b) {
            size_t branchStartIdx = static_cast<size_t>(dist01_(randomEngine_) * (mainPoints.size() * 0.7f));
            Vector3 bStart = mainPoints[branchStartIdx];

            Vector3 mainDir = Subtract(config.end, config.start);
            Vector3 bOffset = {
                distSigned_(randomEngine_) * 2.5f,
                distSigned_(randomEngine_) * 2.0f,
                distSigned_(randomEngine_) * 2.5f
            };
            Vector3 bEnd = Add(bStart, Add(Multiply(0.4f, mainDir), bOffset));

            std::vector<Vector3> branchPoints;
            branchPoints.push_back(bStart);
            GenerateSubdividedLine(bStart, bEnd, config.boltRadius * 0.7f, (std::max)(2, config.subdivisions - 2), branchPoints);

            for (const auto& pt : branchPoints) {
                int idx = AllocateParticle();
                if (idx < 0) break;

                auto& p = particles_[idx];
                p.position = pt;
                p.velocity = { 0.0f, 0.0f, 0.0f };
                p.color = config.glowColor;
                p.scale = config.boltScale * 0.65f;
                p.baseScale = config.boltScale * 0.65f;
                p.lifeTime = config.duration * 0.8f;
                p.currentLife = 0.0f;
                p.intensity = config.intensity * 0.8f;
                p.type = 0;
            }
        }
    }

    // 3. 着弾地点の爆発的スパーク火花 (Impact Burst)
    EmitSparks(config.end, config.impactSparks, config.glowColor, config.sparkSpeed * 1.5f, config.duration * 2.2f, config.boltScale * 0.6f);

    // 4. 着弾地点の強烈なフラッシュグロー
    int glowIdx = AllocateParticle();
    if (glowIdx >= 0) {
        auto& gp = particles_[glowIdx];
        gp.position = config.end;
        gp.velocity = { 0.0f, 0.0f, 0.0f };
        gp.color = config.coreColor;
        gp.scale = config.boltScale * 4.0f;
        gp.baseScale = config.boltScale * 4.0f;
        gp.lifeTime = config.duration * 1.2f;
        gp.currentLife = 0.0f;
        gp.intensity = config.intensity * 2.0f;
        gp.type = 2; // グロー
    }
}

void GpuParticleManager::EmitSparks(
    const Vector3& pos, int count, const Vector4& color, float speed, float lifeTime, float scale)
{
    for (int i = 0; i < count; ++i) {
        int idx = AllocateParticle();
        if (idx < 0) break;

        auto& p = particles_[idx];
        p.position = pos;

        Vector3 dir = {
            distSigned_(randomEngine_),
            dist01_(randomEngine_) * 1.5f + 0.2f,
            distSigned_(randomEngine_)
        };
        if (Length(dir) > 0.001f) {
            dir = Normalize(dir);
        }
        float sp = speed * (0.4f + 0.8f * dist01_(randomEngine_));
        p.velocity = Multiply(sp, dir);

        p.color = color;
        p.scale = scale * (0.6f + 0.8f * dist01_(randomEngine_));
        p.baseScale = p.scale;
        p.lifeTime = lifeTime * (0.6f + 0.8f * dist01_(randomEngine_));
        p.currentLife = 0.0f;
        p.intensity = 3.0f;
        p.type = 1; // スパーク
    }
}

void GpuParticleManager::Clear() {
    for (uint32_t i = 0; i < kMaxParticles; ++i) {
        particles_[i].isActive = false;
        if (mappedGpuData_) {
            mappedGpuData_[i].lifeRatio = 1.0f;
            mappedGpuData_[i].scale = 0.0f;
        }
    }
    activeCount_ = 0;
}

void GpuParticleManager::Update(float deltaTime, const Camera* camera) {
    globalTime_ += deltaTime;

    // 1. シーン定数バッファの更新
    if (sceneData_ && camera) {
        sceneData_->viewProjection = camera->GetViewProjectionMatrix();
        sceneData_->view = camera->GetViewMatrix();

        Matrix4x4 backToFront = MakeRotateYMatrix(std::numbers::pi_v<float>);
        Matrix4x4 billboard = Multiply(backToFront, Inverse(camera->GetViewMatrix()));
        billboard.m[3][0] = 0.0f;
        billboard.m[3][1] = 0.0f;
        billboard.m[3][2] = 0.0f;
        sceneData_->billboardMatrix = billboard;

        sceneData_->cameraPosition = camera->GetWorldPosition();
        sceneData_->time = globalTime_;
    }

    // 2. CPUパーティクル物理・寿命進行 & GPUバッファ書き込み
    activeCount_ = 0;
    for (uint32_t i = 0; i < kMaxParticles; ++i) {
        auto& p = particles_[i];
        if (!p.isActive) {
            mappedGpuData_[i].lifeRatio = 1.0f;
            mappedGpuData_[i].scale = 0.0f;
            continue;
        }

        p.currentLife += deltaTime;
        if (p.currentLife >= p.lifeTime) {
            p.isActive = false;
            mappedGpuData_[i].lifeRatio = 1.0f;
            mappedGpuData_[i].scale = 0.0f;
            continue;
        }

        activeCount_++;
        float lifeRatio = p.currentLife / p.lifeTime;

        if (p.type == 1) {
            p.velocity.y -= 12.0f * deltaTime; // 重力
            p.velocity = Multiply((std::max)(0.0f, 1.0f - 1.8f * deltaTime), p.velocity); // 空気抵抗
            p.position = Add(p.position, Multiply(deltaTime, p.velocity));
            p.scale = p.baseScale * (1.0f - lifeRatio * 0.75f);
        } else if (p.type == 0) {
            p.scale = p.baseScale * (1.0f - lifeRatio * 0.35f);
        } else if (p.type == 2) {
            p.scale = p.baseScale * (1.0f + lifeRatio * 1.5f);
        }

        float decayIntensity = p.intensity * std::exp(-lifeRatio * 4.0f);

        mappedGpuData_[i].position = p.position;
        mappedGpuData_[i].scale = p.scale;
        mappedGpuData_[i].color = p.color;
        mappedGpuData_[i].intensity = decayIntensity;
        mappedGpuData_[i].lifeRatio = lifeRatio;
        mappedGpuData_[i].particleType = p.type;
        mappedGpuData_[i].customParam = 0.0f;
    }
}

void GpuParticleManager::Draw(const Camera* camera) {
    if (!camera) return;

    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
    PSOManager* psoManager = dxCommon_->GetPSO();

    ID3D12PipelineState* pipelineState = psoManager->GetPipelineState(
        device_,
        PrimitiveType::kGpuLightning,
        BlendMode::kAdd,
        D3D12_FILL_MODE_SOLID,
        D3D12_CULL_MODE_NONE
    );

    commandList->SetGraphicsRootSignature(psoManager->GetRootSignature(PrimitiveType::kGpuLightning));
    commandList->SetPipelineState(pipelineState);

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetIndexBuffer(&indexBufferView_);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->SetGraphicsRootConstantBufferView(0, sceneResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(1, srvHandleGPU_);

    commandList->DrawIndexedInstanced(kIndexCount, kMaxParticles, 0, 0, 0);
}

void GpuParticleManager::DrawImGui() {
#ifdef USE_IMGUI
    if (ImGui::TreeNode("GPU Lightning & Particle Manager")) {
        ImGui::Text("Active Particles: %u / %u", activeCount_, kMaxParticles);

        if (ImGui::Button("Trigger Lightning")) {
            EmitLightning(testConfig_);
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear All Particles")) {
            Clear();
        }

        ImGui::Separator();
        ImGui::Text("Test Lightning Settings:");
        ImGui::DragFloat3("Start Pos", &testConfig_.start.x, 0.1f);
        ImGui::DragFloat3("End Pos", &testConfig_.end.x, 0.1f);
        ImGui::ColorEdit4("Core Color", &testConfig_.coreColor.x);
        ImGui::ColorEdit4("Glow Color", &testConfig_.glowColor.x);
        ImGui::SliderFloat("Bolt Radius (Jitter)", &testConfig_.boltRadius, 0.05f, 1.5f);
        ImGui::SliderFloat("Bolt Scale", &testConfig_.boltScale, 0.05f, 1.0f);
        ImGui::SliderInt("Subdivisions", &testConfig_.subdivisions, 3, 9);
        ImGui::SliderInt("Branches", &testConfig_.mainBranches, 0, 6);
        ImGui::SliderInt("Sparks/Seg", &testConfig_.sparksPerSegment, 0, 5);
        ImGui::SliderInt("Impact Sparks", &testConfig_.impactSparks, 0, 200);
        ImGui::SliderFloat("Duration (s)", &testConfig_.duration, 0.05f, 0.8f);
        ImGui::SliderFloat("Intensity (Bloom)", &testConfig_.intensity, 1.0f, 10.0f);
        ImGui::SliderFloat("Spark Speed", &testConfig_.sparkSpeed, 1.0f, 20.0f);

        ImGui::TreePop();
    }
#endif
}

} // namespace Bonjin
