#include "ResultScene.h"

using namespace Bonjin;

void ResultScene::Initialize(Camera* camera) {
	currentSceneType_ = "ResultScene";
	nextSceneType_ = currentSceneType_;
	phase_ = ResultPhase::kFadeIn;
	camera_ = camera;

	clearSprite_ = std::make_unique<Sprite>();
	clearSprite_->Initialize("stage_clear.png");
	// 画面サイズ 1280x720 の中央に配置する
	clearSprite_->Translate() = { 640.0f, 360.0f };
	clearSprite_->Size() = { 1280.0f, 720.0f };
	clearSprite_->Scale() = { 1.0f, 1.0f };
	clearSprite_->Anchor() = { 0.5f, 0.5f, 0.0f };
}

void ResultScene::Unload() {
	clearSprite_.reset();
}

void ResultScene::Update(float deltaTime) {
	(void)deltaTime;
	if (clearSprite_) {
		clearSprite_->Update();
	}

	switch (phase_) {
	case ResultPhase::kFadeIn:
		ChangePhase(ResultPhase::kActive);
		break;
	case ResultPhase::kActive:
		if (Input::GetInstance()->IsTrigger(DIK_SPACE)) {
			ChangePhase(ResultPhase::kFadeOut);
		}
		break;
	case ResultPhase::kFadeOut:
		nextSceneType_ = "TitleScene";
		break;
	}
}

void ResultScene::Draw() {
	if (clearSprite_) {
		//clearSprite_->Draw();
	}
}

SceneType ResultScene::GetNextScene() const {
	return nextSceneType_;
}
