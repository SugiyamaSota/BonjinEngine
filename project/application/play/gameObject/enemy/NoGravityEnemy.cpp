#include "NoGravityEnemy.h"
#include "ImGuiManager.h"
#include "../../logic/Collision.h"
#include "../player/Player.h"
#include <cmath>
#include <numbers>
#include <algorithm>
#include <string>

namespace Bonjin {

void NoGravityEnemy::Initialize(Object3D* model, Camera* camera, const Vector3& position) {
	GameObjectInitConfig config;
	config.physicsType = PhysicsType::Linear; // 重力なし
	config.hasCollider = true;
	config.categoryAttr = kAttributeEnemy;
	config.collisionMask = kAttributePlayer | kAttributeAnchor;
	config.tag = "Enemy";

	Vector3 spawnPos = position;
	spawnPos.y += kSpawnHeight; // 少し高めに出現させる

	BaseEnemy::Initialize(model, camera, spawnPos, config);

	floatingTimer_ = 0.0f;
	expReward_ = 40; // 空中敵は少し経験値を多くする

	chaseAngle_ = 0.0f;
	orbitDirection_ = 1.0f;
	isChaseInitialized_ = false;
	orbitRadius_ = 4.0f;
	orbitSpeed_ = 1.2f;
	minOrbitAngle_ = 0.0f;
	maxOrbitAngle_ = std::numbers::pi_v<float>;
	patrolBaseY_ = spawnPos.y;
}

void NoGravityEnemy::PatrolBehavior() {
	// 左右に移動
	if (lrDirection_ == LRDirection::kRight) {
		velocity_.x = kFlySpeed;
	} else {
		velocity_.x = -kFlySpeed;
	}
	velocity_.y = 0.0f;

	// サイン波でY座標をふわふわさせる
	float offset = std::sin(floatingTimer_ * kFloatSpeed) * kFloatAmplitude;
	worldTransform_.translate.y = patrolBaseY_ + offset;
}

void NoGravityEnemy::ChaseBehavior() {
	Vector3 playerPos = player_->GetWorldPosition();
	Vector3 myPos = GetWorldPosition();

	// 1. 初回の角度設定
	if (!isChaseInitialized_) {
		Vector3 diff = Subtract(myPos, playerPos);
		chaseAngle_ = std::atan2(diff.y, diff.x);
		// 角度を 0 ~ pi にクランプ
		chaseAngle_ = std::clamp(chaseAngle_, minOrbitAngle_, maxOrbitAngle_);
		isChaseInitialized_ = true;
	}

	// 2. 角度の更新
	chaseAngle_ += orbitDirection_ * orbitSpeed_ * (1.0f / 60.0f);

	// 3. 限界角度に達したら反転
	if (chaseAngle_ >= maxOrbitAngle_) {
		chaseAngle_ = maxOrbitAngle_;
		orbitDirection_ = -1.0f;
	} else if (chaseAngle_ <= minOrbitAngle_) {
		chaseAngle_ = minOrbitAngle_;
		orbitDirection_ = 1.0f;
	}

	// 4. 目標座標の計算 (プレイヤーを中心とする円運動)
	Vector3 targetPos;
	targetPos.x = playerPos.x + orbitRadius_ * std::cos(chaseAngle_);
	targetPos.y = playerPos.y + orbitRadius_ * std::sin(chaseAngle_);
	targetPos.z = spawnPosition_.z;

	// 5. 速度ベクトルの設定
	Vector3 toTarget = Subtract(targetPos, myPos);
	float distToTarget = Length(toTarget);
	if (distToTarget > 0.05f) {
		velocity_ = Multiply(kFlySpeed * 1.5f, Normalize(toTarget));
	} else {
		velocity_ = Vector3(0.0f, 0.0f, 0.0f);
	}

	// 6. プレイヤーの方を向いて撃つ
	UpdateChaseAttack();
}

void NoGravityEnemy::UpdateMotion() {
	// 浮遊アニメーション (緩やかに傾く)
	floatingTimer_ += 1.0f / 60.0f;
	float param = std::sin(kPi * 2 * floatingTimer_ / 1.5f);
	worldTransform_.rotate.z = param * 0.1f;
}

void NoGravityEnemy::OnStartChase() {
	isChaseInitialized_ = false; // 追跡開始時に角度の初期設定を促す
}

void NoGravityEnemy::OnLoseChase() {
	isChaseInitialized_ = false;
	// 今の高さを基準にして、追跡前に向いていた方へ飛び出す
	patrolBaseY_ = worldTransform_.translate.y;
	velocity_.x = (lrDirection_ == LRDirection::kRight) ? kFlySpeed : -kFlySpeed;
	velocity_.y = 0.0f;
}

void NoGravityEnemy::OnMapCollision(const CollisionMapInfo& collisionMapinfo) {
	GameObject::OnMapCollision(collisionMapinfo);

	// 壁に当たったら反転
	if (collisionMapinfo.isHitWall_ || collisionMapinfo.isHotTop_ || collisionMapinfo.isLandin_) {
		if (state_ == State::kPatrol) {
			lrDirection_ = (lrDirection_ == LRDirection::kRight) ? LRDirection::kLeft : LRDirection::kRight;
		} else if (state_ == State::kChase) {
			// 追従中は円運動の回転方向を反転
			orbitDirection_ *= -1.0f;
		}
	}
}

void NoGravityEnemy::DrawImGui(int index) {
#ifdef USE_IMGUI
	ImGui::PushID(index);
	const std::string nodeName = MakeImGuiNodeName("NoGravity Enemy", index);

	if (ImGui::TreeNode(nodeName.c_str())) {
		if (!isDead_) {
			Vector3 pos = GetWorldPosition();
			ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);

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

			// 円運動パラメータのImGui調整
			ImGui::SliderFloat("Orbit Radius", &orbitRadius_, 1.0f, 10.0f, "%.1f");
			ImGui::SliderFloat("Orbit Speed", &orbitSpeed_, 0.1f, 5.0f, "%.1f");

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
