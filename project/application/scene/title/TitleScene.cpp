#include "TitleScene.h"

using namespace BonjinEngine;

void TitleScene::Initialize(Camera* camera) {
    // 今のシーンと遷移後シーン(初期値は同じ)
    currentSceneType_ = SceneType::kTitle;
    nextSceneType_ = SceneType::kTitle;

    this->camera_ = camera;

	blackScreenSprite_ = new Sprite();
	blackScreenSprite_->Initialize({ 640.0f,360.0f,0.0f }, Color::Black, { 0.5f,0.5f,0.0f }, { 1280,720 }, "uvChecker.png");

	// フェーズとタイマーを初期化
	phase_ = TitlePhase::kFadeIn;
	phaseTimer_ = 0.0f;

	worldTransform_ = InitializeWorldTransform();
	titleModel_ = new Model;
	titleModel_->LoadModel("title");
	titleModel_->Update(worldTransform_, camera_);

	titleUIModel_ = new Model;
	titleUIModel_->LoadModel("titleUI");
	titleUIModel_->Update(worldTransform_, camera_);

	// 天球
	skydomeModel_ = std::make_unique<Model>();
	skydomeModel_->LoadModel("debugSkydome");
	skydome_ = std::make_unique<Skydome>();
	skydome_->Initialize(skydomeModel_.get(), camera_);
}

TitleScene::~TitleScene() {
	if (titleModel_) {
		delete titleModel_;
	}
	if (titleUIModel_) {
		delete titleUIModel_;
	}
	if (blackScreenSprite_) {
		delete blackScreenSprite_;
	}
}


void TitleScene::Update(float deltaTime) {
    WorldTransform wt = InitializeWorldTransform();
	skydome_->Update();

	titleModel_->Update(worldTransform_, camera_);
	titleUIModel_->Update(worldTransform_, camera_);

	phaseTimer_ += 1.0f / 60.0f;

	switch (phase_) {
	case TitlePhase::kFadeIn:
	{
		// フェードインのアルファ値を計算
		float alpha = 1.0f - min(phaseTimer_ / 2.0f, 1.0f); // 2秒かけてフェードイン
		blackScreenSprite_->Update({ 640.0f,360.0f,0.0f }, Color::Black);
		blackScreenSprite_->SetColor({ 0.0f, 0.0f, 0.0f, alpha });

		// フェードインが完了したら次のフェーズへ
		if (alpha <= 0.0f) {
			phase_ = TitlePhase::kActive;
			phaseTimer_ = 0.0f;
		}
	}
	break;
	case TitlePhase::kActive:
		// スペースキーが押されたらフェードアウト開始
		if (Input::GetInstance()->IsPadTrigger(0) || Input::GetInstance()->IsTrigger(DIK_SPACE)) {
			phase_ = TitlePhase::kFadeOut;
			phaseTimer_ = 0.0f;
		}
		break;
	case TitlePhase::kFadeOut:
	{
		// フェードアウトのアルファ値を計算
		float alpha = min(phaseTimer_ / 2.0f, 1.0f); // 2秒かけてフェードアウト
		blackScreenSprite_->Update({ 640.0f,360.0f,0.0f }, Color::Black);
		blackScreenSprite_->SetColor({ 0.0f, 0.0f, 0.0f, alpha });

		// フェードアウトが完了したらシーン変更
		if (alpha >= 1.0f) {
			nextSceneType_ = SceneType::kGame;
		}
	}
	break;
	}
}

void TitleScene::Draw() {
	// 天球の描画
	skydome_->Draw();
	titleModel_->Draw();
	titleUIModel_->Draw();
	// 黒いスプライトを描画
	blackScreenSprite_->Draw();
}

SceneType TitleScene::GetNextScene() const {
    return nextSceneType_;
}