#include "TitleScene.h"

using namespace Bonjin;

void TitleScene::Initialize(Camera* camera) {
	currentSceneType_ = SceneType::kTitle;
	nextSceneType_ = SceneType::kTitle;

	this->camera_ = camera;

	// フェーズとタイマーを初期化
	phase_ = TitlePhase::kFadeIn;
	phaseTimer_ = 0.0f;

	titleWT_ = InitializeWorldTransform();
	titleModel_ = std::make_unique<Model>();
	titleModel_->LoadModel("title");
	titleModel_->Update(titleWT_, camera_);

	startHUDWT_ = InitializeWorldTransform();
	startHUDModel_ = std::make_unique<Model>();
	startHUDModel_->LoadModel("titleUI");
	startHUDModel_->Update(startHUDWT_, camera_);
}

void TitleScene::Unload() {
	
}

void TitleScene::Update(float deltaTime) {

	titleModel_->Update(titleWT_, camera_);
	startHUDModel_->Update(startHUDWT_, camera_);

	phaseTimer_ += 1.0f / 60.0f;

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

        nextSceneType_ = SceneType::kGame;

        break;
    }
}

void TitleScene::Draw() {

	titleModel_->Draw();
	startHUDModel_->Draw();

}

void TitleScene::DrawSceneImGui() {


}

SceneType TitleScene::GetNextScene() const {
	return nextSceneType_;
}