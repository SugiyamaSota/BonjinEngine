#include "DemoManager.h"

#include "battle/BattleController.h"
#include "input/Input.h"
#include "input/Gamepad.h"

#include "SceneManager.h"

#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace Bonjin {

DemoManager* DemoManager::GetInstance() {
	static DemoManager instance;
	return &instance;
}

void DemoManager::SetDemoMode(bool enabled) {
	isDemoMode_ = enabled;
	// ゲーム中にOFFにしたらその場で操作を返す
	if (!isDemoMode_ && isInGame_) {
		Input::GetInstance()->SetVirtualMode(false);
		Gamepad::GetInstance()->SetIgnoreInput(false);
	}
}

void DemoManager::BeginGame() {
	isInGame_ = true;
	playTimer_ = 0.0f;
	isGiveUpRequested_ = false;
	stuckTimer_ = 0.0f;
	jumpHoldTimer_ = 0.0f;
	jumpCooldownTimer_ = 0.0f;
	shootCooldownTimer_ = 0.0f;
	lockOnTimer_ = 0.0f;
	if (isDemoMode_) {
		Input::GetInstance()->SetVirtualMode(true);
		Gamepad::GetInstance()->SetIgnoreInput(true);
	}
}

void DemoManager::EndGame() {
	isInGame_ = false;
	Input::GetInstance()->SetVirtualMode(false);
	Gamepad::GetInstance()->SetIgnoreInput(false);
}

void DemoManager::UpdateAutoPlay(const BattleController* battleController, float deltaTime) {
	if (!isDemoMode_ || !isInGame_ || !battleController) {
		return;
	}
	const Player* player = battleController->GetPlayer();
	if (!player) {
		return;
	}

	// 長くかかりすぎたらあきらめてタイトルへ戻る
	playTimer_ += deltaTime;
	if (playTimer_ >= maxPlayTime_ && !isGiveUpRequested_) {
		isGiveUpRequested_ = true;
		SceneManager::GetInstance()->RequestSceneChange("TitleScene");
	}

	// ヒットストップ中はプレイヤーが入力を受け付けないので待つ
	if (battleController->IsHitStopActive()) {
		return;
	}

	// 目の前に敵がいたら立ち止まって攻撃する
	const bool isAttacking = UpdateAutoAttack(battleController, deltaTime);

	const Vector3 playerPosition = player->GetPosition();
	const Vector3 goalPosition = battleController->GetGoalPosition();
	const float dx = goalPosition.x - playerPosition.x;
	const float dy = goalPosition.y - playerPosition.y;

	// ゴールの方へ歩く
	const bool moveRight = !isAttacking && dx > arriveDistance_;
	const bool moveLeft = !isAttacking && dx < -arriveDistance_;
	Input* input = Input::GetInstance();
	input->SetVirtualKey(DIK_D, moveRight);
	input->SetVirtualKey(DIK_A, moveLeft);

	// 歩こうとしているのに進めていなければ詰まっている
	const bool isMoving = moveRight || moveLeft;
	if (isMoving && player->IsOnGround() && std::fabs(player->GetVelocity().x) < stuckSpeed_) {
		stuckTimer_ += deltaTime;
	} else {
		stuckTimer_ = 0.0f;
	}

	jumpCooldownTimer_ -= deltaTime;
	const bool isGoalAboveAndNear = dy > jumpHeightThreshold_ && std::fabs(dx) < jumpNearDistance_;
	const bool shouldJump = !isAttacking && player->IsOnGround() && jumpCooldownTimer_ <= 0.0f && jumpHoldTimer_ <= 0.0f &&
		(stuckTimer_ >= stuckTime_ || isGoalAboveAndNear);
	if (shouldJump) {
		jumpHoldTimer_ = jumpHoldTime_;
		jumpCooldownTimer_ = jumpHoldTime_ + jumpCooldown_;
		stuckTimer_ = 0.0f;
	}

	// ジャンプは押した瞬間で判定されるので、少し押し続けてから離す
	if (jumpHoldTimer_ > 0.0f) {
		input->SetVirtualKey(DIK_SPACE, true);
		jumpHoldTimer_ -= deltaTime;
	}
}

bool DemoManager::UpdateAutoAttack(const BattleController* battleController, float deltaTime) {
	const Player* player = battleController->GetPlayer();
	Input* input = Input::GetInstance();

	shootCooldownTimer_ = (std::max)(shootCooldownTimer_ - deltaTime, 0.0f);

	const bool hasLockedOnEnemy = player->GetLockedOnEnemiesCount() > 0;
	lockOnTimer_ = hasLockedOnEnemy ? lockOnTimer_ + deltaTime : 0.0f;

	const bool hasTarget = FindEnemyInFront(battleController) != nullptr;
	const bool canShoot = hasTarget && !player->HasAnchor() && player->GetAnchorStock() > 0;

	// まだロックオンしていない敵が目の前にいればアンカーを撃つ (何体かいれば先に全員ロックオンする)
	if (canShoot) {
		if (shootCooldownTimer_ <= 0.0f) {
			input->SetVirtualKey(DIK_J, true);
			shootCooldownTimer_ = shootInterval_;
		}
		return true;
	}

	// ロックオンした敵を少し見せてから撃破する
	if (hasLockedOnEnemy) {
		if (lockOnTimer_ >= defeatDelay_) {
			input->SetVirtualKey(DIK_L, true);
			lockOnTimer_ = 0.0f;
		}
		return true;
	}

	// 外れて壁に刺さったアンカーが残っていると次が撃てないので、長押しで回収する
	if (player->HasAnchor() && player->GetAnchor().GetStandBy() && !player->GetAnchor().IsDead()) {
		input->SetVirtualKey(DIK_J, true);
		return hasTarget;
	}
	return false;
}

const BaseEnemy* DemoManager::FindEnemyInFront(const BattleController* battleController) const {
	const Player* player = battleController->GetPlayer();
	const MapChipField* mapChipField = battleController->GetMapChipField();
	if (!mapChipField) {
		return nullptr;
	}
	const Vector3 playerPosition = player->GetPosition();
	const float direction = player->IsFacingRight() ? 1.0f : -1.0f;
	constexpr float kAnchorHalfHeight = 0.5f; // アンカーの当たり判定の高さの半分
	constexpr float kWallCheckStep = 0.5f;

	const BaseEnemy* nearestEnemy = nullptr;
	float nearestDistance = attackRange_;
	for (const auto& enemy : battleController->GetEnemies()) {
		if (enemy->GetIsDead() || enemy->GetIsLockedOn()) {
			continue;
		}
		// 向いている側の近くにいるか
		const float distance = (enemy->GetWorldPosition().x - playerPosition.x) * direction;
		if (distance <= 0.0f || distance > nearestDistance) {
			continue;
		}
		// まっすぐ撃ったアンカーが当たる高さか
		const AABB enemyAABB = enemy->GetAABB();
		const float overlap =
			(std::min)(playerPosition.y + kAnchorHalfHeight, enemyAABB.max.y) -
			(std::max)(playerPosition.y - kAnchorHalfHeight, enemyAABB.min.y);
		if (overlap < attackHeightMargin_) {
			continue;
		}
		// 間に壁がないか
		bool isBlocked = false;
		for (float d = kWallCheckStep; d < distance; d += kWallCheckStep) {
			const IndexSet index = mapChipField->GetMapChipIndexSetByPosition(
				{ playerPosition.x + direction * d, playerPosition.y, playerPosition.z });
			if (mapChipField->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipType::kBlock) {
				isBlocked = true;
				break;
			}
		}
		if (isBlocked) {
			continue;
		}
		nearestEnemy = enemy.get();
		nearestDistance = distance;
	}
	return nearestEnemy;
}

void DemoManager::DrawImGui() {
#ifdef USE_IMGUI
	if (ImGui::Begin("Demo")) {
		bool isDemoMode = isDemoMode_;
		if (ImGui::Checkbox("Demo Mode (Auto Loop)", &isDemoMode)) {
			SetDemoMode(isDemoMode);
		}
		ImGui::TextDisabled("ONの間、タイトルとゲームを自動でくり返します");
		ImGui::DragFloat("Title Wait Time (s)", &titleWaitTime_, 0.1f, 0.5f, 60.0f);
		if (ImGui::TreeNode("Auto Play")) {
			ImGui::DragFloat("Max Play Time (s)", &maxPlayTime_, 1.0f, 5.0f, 600.0f);
			ImGui::DragFloat("Arrive Distance", &arriveDistance_, 0.05f, 0.0f, 5.0f);
			ImGui::DragFloat("Stuck Speed", &stuckSpeed_, 0.001f, 0.0f, 0.15f);
			ImGui::DragFloat("Stuck Time", &stuckTime_, 0.01f, 0.0f, 2.0f);
			ImGui::DragFloat("Jump Height Threshold", &jumpHeightThreshold_, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Jump Near Distance", &jumpNearDistance_, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Jump Hold Time", &jumpHoldTime_, 0.01f, 0.02f, 1.0f);
			ImGui::DragFloat("Jump Cooldown", &jumpCooldown_, 0.01f, 0.0f, 2.0f);
			ImGui::DragFloat("Attack Range", &attackRange_, 0.1f, 0.0f, 50.0f);
			ImGui::DragFloat("Attack Height Margin", &attackHeightMargin_, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Shoot Interval", &shootInterval_, 0.01f, 0.05f, 3.0f);
			ImGui::DragFloat("Defeat Delay", &defeatDelay_, 0.01f, 0.0f, 3.0f);
			ImGui::TreePop();
		}
	}
	ImGui::End();
#endif
}

}
