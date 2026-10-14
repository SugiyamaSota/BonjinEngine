#include "GameScene.h"

#include "gameObject/player/PlayerStatusRepository.h"
#include "../result/ResultScene.h"
#include "SceneManager.h"
#include <algorithm>

using namespace Bonjin;

void GameScene::Initialize(Camera* camera)
{
	camera_ = camera;
	phase_ = GamePhase::kPlay;

	// 今のシーンと遷移後シーン(初期値は同じ)
	currentSceneType_ = "GameScene";
	nextSceneType_ = "GameScene";

	// プレイヤーステータスの読み込み
	PlayerStatusRepository::GetInstance()->Load("resources/data/player_status.json");

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

	if (phase_ == GamePhase::kPlay) {
		if (battleController_->IsGoalReached()) {
			ResultScene::SetIsClear(true);
			ChangePhase(GamePhase::kGoal);
			SceneManager::GetInstance()->RequestSceneChange("ResultScene");
		} else if (battleController_->GetPlayer() && battleController_->GetPlayer()->GetStatus().IsDead()) {
			ResultScene::SetIsClear(false);
			ChangePhase(GamePhase::kGoal);
			SceneManager::GetInstance()->RequestSceneChange("ResultScene");
		}
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