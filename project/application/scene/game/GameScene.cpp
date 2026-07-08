#include "GameScene.h"

using namespace Bonjin;

void GameScene::Initialize(Camera* camera)
{
	camera_ = camera;
	phase_ = GamePhase::kStart;

	// 今のシーンと遷移後シーン(初期値は同じ)
	currentSceneType_ = "GameScene";
	nextSceneType_ = "GameScene";

	battleController_ = std::make_unique<BattleController>();
	battleController_->Initialize(camera_, "resources/maps/tutorial.csv");
}

void GameScene::Unload() {
	if (battleController_) {
		battleController_->Unload();
		battleController_.reset();
	}
}

void GameScene::Update(float deltaTime) {
	battleController_->Update(deltaTime);

	if (battleController_->IsGoalReached()) {
		ChangePhase(GamePhase::kGoal);
	}

	switch (phase_) {
	case GamePhase::kStart:
		ChangePhase(GamePhase::kPlay);
		break;
	case GamePhase::kPlay:
		
		break;
	case GamePhase::kGoal:

		nextSceneType_ = "ResultScene";

		break;
	}
}

void GameScene::Draw() {
	battleController_->Draw();
}

SceneType GameScene::GetNextScene() const {
	return nextSceneType_;
}

void GameScene::DrawSceneImGui() {
#ifdef USE_IMGUI
	if (battleController_) {
		battleController_->DrawImGui();
	}
#endif
}