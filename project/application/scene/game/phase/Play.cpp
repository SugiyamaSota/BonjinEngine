#include "Play.h"
#include "Goal.h"
#include "GameOver.h"
#include "../GameScene.h"

#include "../../result/ResultScene.h"
#include "SceneManager.h"
#include "../../../demo/DemoManager.h"

namespace Bonjin::GamePhase {

void Play::Enter(GameScene* scene) {
	auto bc = scene->GetBattleController();
	if (bc) {
		bc->SetHudVisible(true);
		if (!bc->IsGoSignalActive()) {
			bc->TriggerGoSignal();
		}
	}
}

void Play::Update(GameScene* scene, float deltaTime) {
	auto bc = scene->GetBattleController();
	if (!bc) {
		return;
	}

	// デモモードで長くかかりすぎたらタイトルへ戻る (自動操作は DemoPlayerController が命令を出す)
	DemoManager::GetInstance()->UpdatePlayTimer(deltaTime);
	bc->Update(deltaTime);

	if (bc->IsGoalReached()) {
		ResultScene::SetIsClear(true);
		// クリア演出(Flair)の後でGoalフェーズがリザルトへ遷移させる
		scene->ChangePhase(std::make_unique<Goal>());
	} else if (bc->GetPlayer() && bc->GetPlayer()->GetStatus().IsDead()) {
		ResultScene::SetIsClear(false);
		scene->ChangePhase(std::make_unique<GameOver>());
	}
}

void Play::Exit(GameScene* scene) {
	(void)scene;
}

}
