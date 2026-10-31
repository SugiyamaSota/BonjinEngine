#include "TutorialScene.h"
#include "phase/Play.h"
#include "gameObject/player/PlayerStatusRepository.h"
#include <algorithm>

using namespace Bonjin;

void TutorialScene::Initialize(Camera* camera) {
	currentSceneType_ = "TutorialScene";
	nextSceneType_ = currentSceneType_;
	camera_ = camera;

	// プレイヤーステータスの読み込み
	PlayerStatusRepository::GetInstance()->Load("resources/data/player_status.json");

	battleController_ = std::make_unique<BattleController>();
	battleController_->Initialize(camera_, "resources/maps/tutorial.json");

	tutorialManager_ = std::make_unique<TutorialManager>();
	tutorialManager_->Initialize("resources/data/tutorial.json");

	tutorialEditor_ = std::make_unique<TutorialEditor>();
	tutorialEditor_->Initialize(tutorialManager_.get());

	battleController_->TriggerGoSignal();

	// 初期フェーズを設定
	ChangePhase(std::make_unique<TutorialPhase::Play>());
}

void TutorialScene::Unload() {
	if (currentPhase_) {
		currentPhase_->Exit(this);
		currentPhase_.reset();
	}
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
	}

	if (currentPhase_) {
		currentPhase_->Update(this, deltaTime);
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
