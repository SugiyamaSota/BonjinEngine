#include "DemoManager.h"

#include "battle/BattleController.h"
#include "input/Input.h"
#include "input/Gamepad.h"

#include "SceneManager.h"

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

	const Vector3 playerPosition = player->GetPosition();
	const Vector3 goalPosition = battleController->GetGoalPosition();
	const float dx = goalPosition.x - playerPosition.x;
	const float dy = goalPosition.y - playerPosition.y;

	// ゴールの方へ歩く
	const bool moveRight = dx > arriveDistance_;
	const bool moveLeft = dx < -arriveDistance_;
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
	const bool shouldJump = player->IsOnGround() && jumpCooldownTimer_ <= 0.0f && jumpHoldTimer_ <= 0.0f &&
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
			ImGui::DragFloat("Stuck Speed", &stuckSpeed_, 0.05f, 0.0f, 5.0f);
			ImGui::DragFloat("Stuck Time", &stuckTime_, 0.01f, 0.0f, 2.0f);
			ImGui::DragFloat("Jump Height Threshold", &jumpHeightThreshold_, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Jump Near Distance", &jumpNearDistance_, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Jump Hold Time", &jumpHoldTime_, 0.01f, 0.02f, 1.0f);
			ImGui::DragFloat("Jump Cooldown", &jumpCooldown_, 0.01f, 0.0f, 2.0f);
			ImGui::TreePop();
		}
	}
	ImGui::End();
#endif
}

}
