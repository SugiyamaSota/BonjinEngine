#include "TitleScene.h"
#include "input/Input.h"
#include <cmath>

using namespace Bonjin;

void TitleScene::Initialize(Camera* camera) {
	currentSceneType_ = "TitleScene";
	nextSceneType_ = "TitleScene";
	phase_ = TitlePhase::kFadeIn;
	camera_ = camera;
	timer_ = 0.0f;

	// タイトルテキスト「Anchor」の生成と配置
	titleText_ = std::make_unique<TextSprite>();
	titleText_->Initialize();
	titleText_->SetAnchor({ 0.5f, 0.5f, 0.0f });
	titleText_->SetText(L"Anchor", 80, RGB(255, 255, 255));
	titleText_->SetTranslate({ 640.0f, 260.0f });

	// スタート案内テキスト「SpaceToStart」の生成と配置
	startText_ = std::make_unique<TextSprite>();
	startText_->Initialize();
	startText_->SetAnchor({ 0.5f, 0.5f, 0.0f });
	startText_->SetText(L"SpaceToStart", 36, RGB(220, 220, 220));
	startText_->SetTranslate({ 640.0f, 480.0f });
}

void TitleScene::Unload() {
	titleText_.reset();
	startText_.reset();
}

void TitleScene::Update(float deltaTime) {
	timer_ += deltaTime;

	if (titleText_) {
		titleText_->Update();
	}

	if (startText_) {
		// スタート案内テキストの点滅演出
		float alpha = 0.5f + 0.5f * std::sin(timer_ * 4.0f);
		startText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
		startText_->Update();
	}

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
	if (titleText_) {
		titleText_->Draw();
	}
	if (startText_) {
		startText_->Draw();
	}
}

SceneType TitleScene::GetNextScene() const {
	return nextSceneType_;
}