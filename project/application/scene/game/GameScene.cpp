#include "GameScene.h"

using namespace Bonjin;

void GameScene::Initialize(Camera* camera) {
	currentSceneType_ = SceneType::kGame;
	nextSceneType_ = currentSceneType_;
	phase_ = GamePhase::kStart;
	camera_ = camera;

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
		nextSceneType_ = SceneType::kResult;
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
	battleController_->DrawImGui();
}
