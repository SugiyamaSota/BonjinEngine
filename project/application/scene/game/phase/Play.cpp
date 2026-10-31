#include "Play.h"
#include "Goal.h"
#include "GameOver.h"
#include "../GameScene.h"

#include "../../result/ResultScene.h"
#include "SceneManager.h"

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

	bc->Update(deltaTime);

	if (bc->IsGoalReached()) {
		ResultScene::SetIsClear(true);
		scene->ChangePhase(std::make_unique<Goal>());
		SceneManager::GetInstance()->RequestSceneChange("ResultScene");
	} else if (bc->GetPlayer() && bc->GetPlayer()->GetStatus().IsDead()) {
		ResultScene::SetIsClear(false);
		scene->ChangePhase(std::make_unique<GameOver>());
	}
}

void Play::Exit(GameScene* scene) {
	(void)scene;
}

}
