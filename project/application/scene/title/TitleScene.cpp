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

	UpdatePhaseTimer(deltaTime);

	// タイトルロゴの揺れ（ゆっくり、大きく）
	float titleY = std::sin(phaseTimer_ * 2.0f) * 0.3f;
	titleWT_.translate.y = 0.5f + titleY;

	float hudY;
	
	if (phase_ != TitlePhase::kFadeOut) {
		// スタートHUDの揺れ（少し速く、小さく）
		hudY = std::sin((phaseTimer_ + 0.5f) * 3.0f) * 0.15f;
		startHUDWT_.translate.y = -1.0f + hudY;
	}

    // フェーズに応じた処理
    switch (phase_) {
    case TitlePhase::kFadeIn:

		// フェードインのアルファ値を計算
		fadeIOAlpha_ = 1.0f - min(phaseTimer_ / 1.0f, 1.0f);
		// フェードインが完了したら次のフェーズへ
		if (fadeIOAlpha_ <= 0.0f) {
			ChangePhase(TitlePhase::kActive);
		}
      
        break;

    case TitlePhase::kActive:

		

        if (Input::GetInstance()->IsTrigger(DIK_SPACE)) {
            ChangePhase(TitlePhase::kFadeOut);

			// スペース押したら初速を追加
			hudRotationSpeed_ = 0.2f;
			hudRotationY_ = 0.0f;

        }
        break;

    case TitlePhase::kFadeOut:

		// 回転速度を角度に加算
		hudRotationY_ += hudRotationSpeed_;

		// 速度減衰
		hudRotationSpeed_ *= 0.98f; 

		// 3角度をモデルのトランスフォームに適用
		startHUDWT_.rotate.y = hudRotationY_;

		// フェードインのアルファ値を計算
		fadeIOAlpha_ = min(phaseTimer_ / 1.0f, 1.0f);
		// フェードインが完了したら次のフェーズへ
		if (fadeIOAlpha_ >= 1.0f) {
			nextSceneType_ = SceneType::kGame;
		}

        break;
    }

	titleModel_->Update(titleWT_, camera_);
	startHUDModel_->Update(startHUDWT_, camera_);

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