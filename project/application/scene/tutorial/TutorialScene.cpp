#include "TutorialScene.h"
#include "gameObject/player/PlayerStatusRepository.h"
#include "SceneManager.h"
#include <algorithm>

using namespace Bonjin;

void TutorialScene::Initialize(Camera* camera) {
	currentSceneType_ = "TutorialScene";
	nextSceneType_ = currentSceneType_;
	phase_ = TutorialPhase::kPlay;
	camera_ = camera;

	// プレイヤーステータスの読み込み
	PlayerStatusRepository::GetInstance()->Load("resources/data/player_status.json");

	battleController_ = std::make_unique<BattleController>();
	battleController_->Initialize(camera_, "resources/maps/tutorial.csv");

	tutorialManager_ = std::make_unique<TutorialManager>();
	tutorialManager_->Initialize("resources/data/tutorial.json");

	tutorialEditor_ = std::make_unique<TutorialEditor>();
	tutorialEditor_->Initialize(tutorialManager_.get());
}

void TutorialScene::Unload() {
	if (tutorialEditor_) {
		tutorialEditor_.reset();
	}
	if (tutorialManager_) {
		tutorialManager_.reset();
	}
	if (battleController_) {
		battleController_->Unload();
		battleController_.reset();
	}
}

void TutorialScene::Update(float deltaTime) {
	battleController_->Update(deltaTime);

	if (tutorialEditor_) {
		tutorialEditor_->Update(deltaTime);
	}

	if (tutorialManager_) {
		tutorialManager_->Update(battleController_.get(), deltaTime);
		if (tutorialManager_->IsCompleted() && phase_ != TutorialPhase::kComplete) {
			ChangePhase(TutorialPhase::kComplete);
			SceneManager::GetInstance()->RequestSceneChange("GameScene");
		}
	}
}

void TutorialScene::Draw() {
	battleController_->Draw();
	if (tutorialManager_) {
		tutorialManager_->Draw();
	}
}

SceneType TutorialScene::GetNextScene() const {
	return nextSceneType_;
}

void TutorialScene::DrawSceneImGui() {
#ifdef USE_IMGUI
	if (battleController_) {
		battleController_->DrawImGui();
	}
	if (tutorialEditor_) {
		tutorialEditor_->DrawImGui();
	} else if (tutorialManager_) {
		tutorialManager_->DrawImGui();
	}
#endif
}
