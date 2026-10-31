#include "Active.h"
#include "Dive.h"
#include "../TitleScene.h"

#include "input/Input.h"
#include "input/Gamepad.h"

namespace Bonjin::TitlePhase {

void Active::Enter(TitleScene* scene) {
	(void)scene;
}

void Active::Update(TitleScene* scene, float deltaTime) {
	(void)deltaTime;

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

}
