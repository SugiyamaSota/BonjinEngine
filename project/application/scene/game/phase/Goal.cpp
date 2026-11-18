#include "Goal.h"
#include "../GameScene.h"

#include "SceneManager.h"
#include "../../../demo/DemoManager.h"

namespace Bonjin::GamePhase {

void Goal::Enter(GameScene* scene) {
	isSceneChangeRequested_ = false;
	elapsedTime_ = 0.0f;
	if (auto bc = scene->GetBattleController()) {
		bc->StartClearPerformance();
	}
}

void Goal::Update(GameScene* scene, float deltaTime) {
	elapsedTime_ += deltaTime;

	auto bc = scene->GetBattleController();
	// 遷移を頼んだ後も、暗転しきるまでは演出を動かし続ける
	const bool isFinished = bc ? bc->UpdateClearPerformance(deltaTime) : true;

	if (!isSceneChangeRequested_ && (isFinished || elapsedTime_ >= kMaxPerformanceTime)) {
		isSceneChangeRequested_ = true;
		// デモモードはリザルトを出さずにタイトルへ戻ってループする
		SceneManager::GetInstance()->RequestSceneChange(DemoManager::GetInstance()->IsDemoMode() ? "TitleScene" : "ResultScene");
	}
}

void Goal::Exit(GameScene* scene) {
	(void)scene;
}

}
