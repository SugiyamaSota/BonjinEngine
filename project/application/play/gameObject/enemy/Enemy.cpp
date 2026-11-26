#include "Enemy.h"
#include "ImGuiManager.h"
#include "../../logic/Collision.h"
#include "../player/Player.h"
#include <cmath>
#include <string>

namespace Bonjin {

void Enemy::Initialize(Object3D* model, Camera* camera, const Vector3& position) {
	GameObjectInitConfig config;
	config.physicsType = PhysicsType::Gravity;
	config.hasCollider = true;
	config.categoryAttr = kAttributeEnemy;
	config.collisionMask = kAttributePlayer | kAttributeAnchor;
	config.tag = "Enemy";

	BaseEnemy::Initialize(model, camera, position, config);
}

void Enemy::PatrolBehavior() {
	// 巡回時は何もしない（ApplyPhysicsAndMovementで反転処理などを行う）
}

void Enemy::ChaseBehavior() {
	// 立ち止まって撃つ
	velocity_.x = 0.f;
	UpdateChaseAttack();
}

void Enemy::UpdateMotion() {
	// 歩くときに左右に揺れる
	walkTimer_ += 1.0f / 60.0f;

	float param = std::sin(kPi * 2 * walkTimer_ / kWalkTimer);
	float radian = kWalkMotionAngleStart + kWalkMotionAngleEnd * (param + 1.0f) / 2.0f;
	worldTransform_.rotate.z = radian * (kPi / 180.0f);

	if (walkTimer_ >= kWalkTimer) {
		walkTimer_ = 0.0f;
	}
}

void Enemy::OnLoseChase() {
	// 追跡前に向いていた方へ歩き出す
	velocity_.x = (lrDirection_ == LRDirection::kRight) ? kWalkSpeed : -kWalkSpeed;
}

void Enemy::OnMapCollision(const CollisionMapInfo& collisionMapinfo) {
	GameObject::OnMapCollision(collisionMapinfo);

	if (collisionMapinfo.isHitWall_) {
		if (state_ == State::kPatrol) {
			velocity_.x *= -1.0f;
			lrDirection_ =
				velocity_.x > 0.0f ? LRDirection::kRight : LRDirection::kLeft;
		} else {
			velocity_.x = 0.0f;
		}
	}
}

void Enemy::DrawImGui(int index) {
#ifdef USE_IMGUI
	ImGui::PushID(index);
	const std::string nodeName = MakeImGuiNodeName("Walking Enemy", index);

	if (ImGui::TreeNode(nodeName.c_str())) {
		if (!isDead_) {
			Vector3 pos = GetWorldPosition();
			ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
			ImGui::Text("On Ground: %s", IsOnGround() ? "true" : "false");

			if (player_) {
				Vector3 pPos = player_->GetWorldPosition();
				float dist = Length(Subtract(pPos, pos));
				ImGui::Text("Distance to Player: %.2f", dist);
			}

			float sRad = searchRadius_;
			if (ImGui::SliderFloat("Search Radius", &sRad, 1.0f, 30.0f, "%.1f")) {
				searchRadius_ = sRad;
			}

			float lRad = loseRadius_;
			if (ImGui::SliderFloat("Lose Radius", &lRad, 1.0f, 30.0f, "%.1f")) {
				loseRadius_ = lRad;
			}

			int expRew = expReward_;
			if (ImGui::SliderInt("Exp Reward", &expRew, 0, 500)) {
				expReward_ = expRew;
			}
		} else {
			ImGui::Text("Dead...");
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
#else
	(void)index;
#endif
}

} // namespace Bonjin
