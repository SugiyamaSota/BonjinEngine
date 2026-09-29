#include "TutorialScene.h"

using namespace Bonjin;

void TutorialScene::Initialize(Camera* camera) {
	currentSceneType_ = "TutorialScene";
	nextSceneType_ = currentSceneType_;
	phase_ = TutorialPhase::kStart;
	camera_ = camera;

	battleController_ = std::make_unique<BattleController>();
	battleController_->Initialize(camera_, "resources/maps/tutorial.csv");

	tutorialManager_ = std::make_unique<TutorialManager>();
	tutorialManager_->Initialize();
}

void TutorialScene::Unload() {
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

	if (tutorialManager_) {
		tutorialManager_->Update(battleController_.get(), deltaTime);
		if (tutorialManager_->IsCompleted()) {
			ChangePhase(TutorialPhase::kComplete);
		}
	}

	switch (phase_) {
	case TutorialPhase::kStart:
		ChangePhase(TutorialPhase::kPlay);
		break;
	case TutorialPhase::kPlay:
		break;
	case TutorialPhase::kComplete:
		nextSceneType_ = "GameScene";
		break;
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
	if (tutorialManager_) {
		tutorialManager_->DrawImGui();
	}
#endif
}

