#include "TutorialScene.h"

using namespace Bonjin;

void TutorialScene::Initialize(Camera* camera) {
	currentSceneType_ = "TutorialScene";
	nextSceneType_ = currentSceneType_;
	phase_ = TutorialPhase::kStart;
	camera_ = camera;

	battleController_ = std::make_unique<BattleController>();
	battleController_->Initialize(camera_, "resources/maps/tutorial.csv");
}

void TutorialScene::Unload() {
	if (battleController_) {
		battleController_->Unload();
		battleController_.reset();
	}
}

void TutorialScene::Update(float deltaTime) {
	battleController_->Update(deltaTime);

	switch (phase_) {
	case TutorialPhase::kStart:
		ChangePhase(TutorialPhase::kPlay);
		break;
	case TutorialPhase::kPlay:
		// チュートリアル固有の進行条件をここへ追加する。
		break;
	case TutorialPhase::kComplete:
		nextSceneType_ = "GameScene";
		break;
	}
}

void TutorialScene::Draw() {
	battleController_->Draw();
	// チュートリアル固有の案内表示をここへ追加する。
}

SceneType TutorialScene::GetNextScene() const {
	return nextSceneType_;
}

void TutorialScene::DrawSceneImGui() {
	battleController_->DrawImGui();
}
