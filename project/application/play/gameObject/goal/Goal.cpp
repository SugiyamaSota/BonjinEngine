#include "Goal.h"
#include "ImGuiManager.h"
#include "ParticleManager.h"
#include "../../logic/CollisionConfig.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Bonjin;

namespace {
Vector4 LerpColor(const Vector4& a, const Vector4& b, float t) {
	return Vector4{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t };
}
}

void Goal::CreatePortalParticleGroups() {
	ParticleManager* particleManager = ParticleManager::GetInstance();
	// ゴールのポータル (光の柱 + 昇るリング)
	particleManager->CreateParticleGroup(kPortalPillarGroup, ModelBuilder::ModelType::kCylinder, "resources/textures/portalPillar.png");
	particleManager->CreateParticleGroup(kPortalRingGroup, ModelBuilder::ModelType::kRing, "resources/textures/portalRing.png");
	// ポータルはカメラの方を向かせず、ワールドの向きのまま置く (リングは床と水平、柱はまっすぐ立てる)。
	// 水平なリングはカメラより上にあると裏側から見えるので両面を描く
	particleManager->SetUseBillboard(kPortalPillarGroup, false);
	particleManager->SetUseBillboard(kPortalRingGroup, false);
	particleManager->SetCullMode(kPortalPillarGroup, CullMode::kNone);
	particleManager->SetCullMode(kPortalRingGroup, CullMode::kNone);
}

Goal::~Goal() {
	// 柱のパーティクルの updateFunc は this を参照しているので、自分より長生きさせない
	ClearPortalParticles();
}

void Goal::Initialize(Camera* camera, const Vector3& position) {
	// ゴールの転送パッド (六角柱。テクスチャはmtlで指定した goalPad.png)
	goalModel_ = std::make_unique<Object3D>();
	goalModel_->LoadModel("goal", "goalPad.obj");
	goalModel_->SetEnableEnableEnvironmentMap(false);
	goalModel_->SetCullMode(CullMode::kNone); // 閉じた立体なので両面描画でも見た目は同じ (面の向きの違いに左右されない)

	GameObjectInitConfig config;
	config.physicsType = PhysicsType::None;
	config.hasCollider = true;
	config.categoryAttr = kAttributeGoal;
	config.collisionMask = kAttributePlayer;
	config.tag = "Goal";

	GameObject::Initialize(goalModel_.get(), camera, position, config);
	SetWidth(kColliderSize);
	SetHeight(kColliderSize);

	// 見た目はマスの中心ではなく床の上に置く (当たり判定はマスの中心のまま)
	modelTransform_ = InitializeWorldTransform();
	modelTransform_.scale = { kPadScale, kPadScale, kPadScale };
	modelTransform_.translate = position;
	modelTransform_.translate.y += -kMapBlockHalfHeight + kPadHalfHeight * kPadScale;

	portalColor_ = portalBaseColor_;
	ringTimer_ = 0.0f;
	portalTime_ = 0.0f;
	clearPulse_ = 0.0f;
	lightningBlend_ = 0.0f;
}

void Goal::Update(float deltaTime) {
	GameObject::Update();
	// GameObject::Update() の後に、見た目の位置で描画行列を上書きする
	if (goalModel_ && camera_) {
		goalModel_->Update(modelTransform_, camera_);
	}
	UpdatePortal(deltaTime);
}

void Goal::DrawImGui() {
#ifdef USE_IMGUI
	if (ImGui::TreeNode("Goal Portal")) {
		ParticleManager* particleManager = ParticleManager::GetInstance();
		ImGui::ColorEdit3("Base Color", &portalBaseColor_.x);
		ImGui::ColorEdit3("Current Color", &portalColor_.x, ImGuiColorEditFlags_NoInputs);
		ImGui::DragFloat("Color Change Speed", &portalColorChangeSpeed_, 0.1f, 0.1f, 60.0f);
		ImGui::DragFloat("Pillar Radius", &pillarRadius_, 0.01f, 0.05f, 5.0f);
		ImGui::DragFloat("Pillar Height", &pillarHeight_, 0.05f, 0.1f, 20.0f);
		ImGui::DragFloat("Pillar Pulse Radius Scale", &pillarPulseRadiusScale_, 0.01f, 1.0f, 5.0f, "x%.2f");
		ImGui::DragFloat("Pillar Pulse Height Scale", &pillarPulseHeightScale_, 0.01f, 1.0f, 5.0f, "x%.2f");
		ImGui::DragFloat("Blink Speed", &blinkSpeed_, 0.05f, 0.0f, 10.0f, "%.2f /s");
		ImGui::SliderFloat("Blink Min", &blinkMin_, 0.0f, 1.0f);
		ImGui::DragFloat("Ring Interval", &ringInterval_, 0.01f, 0.05f, 3.0f, "%.2f s");
		ImGui::DragFloat("Ring Life Time", &ringLifeTime_, 0.01f, 0.1f, 5.0f, "%.2f s");
		ImGui::DragFloat("Ring Scale", &ringScale_, 0.01f, 0.1f, 5.0f);
		ImGui::SliderAngle("Ring Tilt", &ringTilt_, 0.0f, 90.0f); // 90で床と水平
		ImGui::Text("Pillar: %u  Ring: %u",
			particleManager->GetActiveCount(kPortalPillarGroup),
			particleManager->GetActiveCount(kPortalRingGroup));
		if (ImGui::Button("Re-emit Pillar")) {
			particleManager->Clear(kPortalPillarGroup);
			EmitPortalPillar();
		}
		ImGui::TreePop();
	}
#endif
}

void Goal::ClearPortalParticles() {
	ParticleManager* particleManager = ParticleManager::GetInstance();
	particleManager->Clear(kPortalPillarGroup);
	particleManager->Clear(kPortalRingGroup);
}

Vector3 Goal::GetPortalBase() const {
	// 台座(goalPad)の上面の中心
	Vector3 base = modelTransform_.translate;
	base.y += kPadHalfHeight * kPadScale;
	return base;
}

void Goal::EmitPortalPillar() {
	ParticleConfig pillar{};
	pillar.position = GetPortalBase(); // シリンダーは底が原点なので台座の上面に立つ
	// シリンダーモデルは半径1・高さ2
	pillar.scale = { pillarRadius_, pillarHeight_ / 2.0f, pillarRadius_ };
	pillar.color = portalColor_;
	// 透明度は寿命で自動的に下がるので、寿命をとても長くして出しっぱなしにする (シーンを抜けるときに消す)
	pillar.lifeTime = 100000.0f;
	pillar.updateFunc = [this](ParticleData& particle, float) {
		// 明るさを sin で上下させて点滅させる (加算合成なので色の明るさ = 光の強さ)
		const float wave = 0.5f + 0.5f * std::sin(portalTime_ * blinkSpeed_ * 2.0f * std::numbers::pi_v<float>);
		const float brightness = blinkMin_ + (1.0f - blinkMin_) * wave;
		particle.color.x = portalColor_.x * brightness;
		particle.color.y = portalColor_.y * brightness;
		particle.color.z = portalColor_.z * brightness;
		// クリア演出でカメラが寄るのに合わせて、太く・少し高くなって戻る
		const float radius = pillarRadius_ * (1.0f + (pillarPulseRadiusScale_ - 1.0f) * clearPulse_);
		const float height = pillarHeight_ * (1.0f + (pillarPulseHeightScale_ - 1.0f) * clearPulse_);
		particle.transform.scale = { radius, height / 2.0f, radius };
		particle.transform.translate = GetPortalBase();
	};
	ParticleManager::GetInstance()->Emit(kPortalPillarGroup, pillar);
}

void Goal::EmitPortalRing() {
	const Vector3 base = GetPortalBase();
	const float height = pillarHeight_;
	const float scale = ringScale_;

	ParticleConfig ring{};
	ring.position = base;
	ring.rotate = { ringTilt_, 0.0f, 0.0f }; // ワールドの向きのまま置くグループなので、90度倒すと床と水平になる
	ring.scale = { scale, scale, 1.0f };
	ring.color = portalColor_;
	ring.lifeTime = ringLifeTime_;
	ring.updateFunc = [base, height, scale](ParticleData& particle, float) {
		// 下から上へ (最初は速く、上に行くほどゆっくり)。透明度は寿命で自動的に下がる
		const float t = particle.lifeTime > 0.0f ? (std::min)(particle.currentTime / particle.lifeTime, 1.0f) : 1.0f;
		const float eased = 1.0f - (1.0f - t) * (1.0f - t);
		particle.transform.translate = { base.x, base.y + height * eased, base.z };
		// 上に行くほど少しすぼまる
		const float s = scale * (1.0f - 0.25f * t);
		particle.transform.scale = { s, s, 1.0f };
	};
	ParticleManager::GetInstance()->Emit(kPortalRingGroup, ring);
}

void Goal::UpdatePortal(float deltaTime) {
	if (deltaTime <= 0.0f) {
		return;
	}
	UpdatePortalColor(deltaTime);
	portalTime_ += deltaTime;
	ringTimer_ -= deltaTime;
	if (ringTimer_ <= 0.0f) {
		ringTimer_ = (std::max)(ringInterval_, 0.05f);
		EmitPortalRing();
	}
}

void Goal::UpdatePortalColor(float deltaTime) {
	const float blend = (std::clamp)(lightningBlend_, 0.0f, 1.0f);
	const Vector4 target = LerpColor(portalBaseColor_, lightningColor_, blend);

	// 一瞬で切り替えず、すっと目標の色へ近づける
	const float t = 1.0f - std::exp(-portalColorChangeSpeed_ * deltaTime);
	portalColor_ = LerpColor(portalColor_, target, t);
	portalColor_.w = 1.0f;
}
