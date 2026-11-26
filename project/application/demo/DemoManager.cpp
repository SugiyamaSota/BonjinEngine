#include "DemoManager.h"

#include "SceneManager.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace Bonjin {

DemoManager* DemoManager::GetInstance() {
	static DemoManager instance;
	return &instance;
}

void DemoManager::SetDemoMode(bool enabled) {
	// ゲーム中にOFFにしたら、DemoPlayerController がその場で人の操作に戻す
	isDemoMode_ = enabled;
}

void DemoManager::BeginGame() {
	isInGame_ = true;
	playTimer_ = 0.0f;
	isGiveUpRequested_ = false;
}

void DemoManager::EndGame() {
	isInGame_ = false;
}

void DemoManager::UpdatePlayTimer(float deltaTime) {
	if (!isDemoMode_ || !isInGame_) {
		return;
	}
	// 長くかかりすぎたらあきらめてタイトルへ戻る
	playTimer_ += deltaTime;
	if (playTimer_ >= maxPlayTime_ && !isGiveUpRequested_) {
		isGiveUpRequested_ = true;
		SceneManager::GetInstance()->RequestSceneChange("TitleScene");
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
			DemoAutoPlaySettings& s = autoPlaySettings_;
			ImGui::DragFloat("Max Play Time (s)", &maxPlayTime_, 1.0f, 5.0f, 600.0f);
			ImGui::DragFloat("Arrive Distance", &s.arriveDistance, 0.05f, 0.0f, 5.0f);
			ImGui::DragFloat("Stuck Speed", &s.stuckSpeed, 0.001f, 0.0f, 0.15f);
			ImGui::DragFloat("Stuck Time", &s.stuckTime, 0.01f, 0.0f, 2.0f);
			ImGui::DragFloat("Jump Height Threshold", &s.jumpHeightThreshold, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Jump Near Distance", &s.jumpNearDistance, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Jump Hold Time", &s.jumpHoldTime, 0.01f, 0.02f, 1.0f);
			ImGui::DragFloat("Jump Cooldown", &s.jumpCooldown, 0.01f, 0.0f, 2.0f);
			ImGui::DragFloat("Attack Range", &s.attackRange, 0.1f, 0.0f, 50.0f);
			ImGui::DragFloat("Attack Height Margin", &s.attackHeightMargin, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Shoot Interval", &s.shootInterval, 0.01f, 0.05f, 3.0f);
			ImGui::DragFloat("Defeat Delay", &s.defeatDelay, 0.01f, 0.0f, 3.0f);
			ImGui::TreePop();
		}
	}
	ImGui::End();
#endif
}

}
