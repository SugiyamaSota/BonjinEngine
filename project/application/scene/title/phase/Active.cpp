#include "Active.h"
#include "Dive.h"
#include "../TitleScene.h"

#include "input/Input.h"
#include "input/Gamepad.h"
#include "../../../demo/DemoManager.h"

namespace Bonjin::TitlePhase {

void Active::Enter(TitleScene* scene) {
	(void)scene;
	demoTimer_ = 0.0f;
}

void Active::Update(TitleScene* scene, float deltaTime) {
	// デモモードなら一定時間で勝手にゲームへ進む
	auto demo = DemoManager::GetInstance();
	if (demo->IsDemoMode()) {
		demoTimer_ += deltaTime;
		if (demoTimer_ >= demo->GetTitleWaitTime()) {
			scene->ChangePhase(std::make_unique<Dive>("GameScene"));
		}
		return;
	}
	demoTimer_ = 0.0f;

	// スペースキーまたはゲームパッドAボタンでダイブ突入開始
	if (Input::GetInstance()->IsTrigger(DIK_SPACE) || Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_A)) {
		// 選択中のメニューに応じて遷移先を決定（GameScene または TutorialScene）
		std::string targetScene = "GameScene";
		if (scene->GetUI() && scene->GetUI()->GetSelectedMenuItem() == TitleUI::TitleMenuItem::kTutorial) {
			targetScene = "TutorialScene";
		}

		scene->ChangePhase(std::make_unique<Dive>(targetScene));
	}
}

void Active::Exit(TitleScene* scene) {
	(void)scene;
}

void Active::DrawImGui(TitleScene* scene) {
	(void)scene;
	DemoManager::GetInstance()->DrawImGui();
}

}
