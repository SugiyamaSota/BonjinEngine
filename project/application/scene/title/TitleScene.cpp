#include "TitleScene.h"

using namespace Bonjin;

void TitleScene::Initialize(Camera* camera) {
	currentSceneType_ = "TitleScene";
	nextSceneType_ = "TitleScene";

	this->camera_ = camera;

	
}

void TitleScene::Unload() {
	
}

void TitleScene::Update(float deltaTime) {
    // フェーズに応じた処理
    switch (phase_) {
    case TitlePhase::kFadeIn:
        // フェード演出など
        ChangePhase(TitlePhase::kActive);
        break;

    case TitlePhase::kActive:

        if (Input::GetInstance()->IsTrigger(DIK_SPACE)) {
            ChangePhase(TitlePhase::kFadeOut);
        }
        break;

    case TitlePhase::kFadeOut:

        nextSceneType_ = "GameScene";

        break;
    }
}

void TitleScene::Draw() {
}



SceneType TitleScene::GetNextScene() const {
	return nextSceneType_;
}