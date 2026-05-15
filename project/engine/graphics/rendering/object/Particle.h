#pragma once
#include "BaseObject.h" // 継承のため
#include <random>
#include <functional>

class Camera;
struct ParticleData;
using ParticleUpdateFunc = std::function<void(ParticleData&, float deltaTime)>;

struct ParticleData {
	WorldTransform transform;
	float lifeTime;
	float currentTime;
	Vector3 velocity;
	Vector3 acceleration;
	Vector4 color;
	ParticleUpdateFunc updateFunc;
};

// BaseObject を継承
class Particle : public BaseObject {
public:
	Particle();
	~Particle() override; // overrideを明示

	void Update(Camera* camera);
	void Draw() override; // BaseObjectの純粋仮想関数を実装

	void Emit(const Vector3& position, const Vector3& velocity, const Vector4& color, float lifetime, ParticleUpdateFunc func);

	void DrawImGui();

private:
	// BaseObject の SetupResources を実装
	void SetupResources() override;

	// --- Particle 固有のメンバ ---
	static const uint32_t kNumInstance_ = 1000;
	std::mt19937 randomEngine_;

	// インスタンシング用（WVPの代わり）
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_ = nullptr;
	ParticleForGPU* instancingData_ = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE srvhandleGPU_;
	int srvIndex_ = 0;

	ParticleData particles_[kNumInstance_];
};