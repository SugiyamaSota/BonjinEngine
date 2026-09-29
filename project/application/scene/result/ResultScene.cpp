#include "ResultScene.h"
#include "input/Input.h"
#include <cmath>

using namespace Bonjin;

void ResultScene::Initialize(Camera* camera) {
	currentSceneType_ = "ResultScene";
	nextSceneType_ = currentSceneType_;
	phase_ = ResultPhase::kFadeIn;
	camera_ = camera;
	timer_ = 0.0f;

	// 「GameClear」テキストスプライトの生成（金色・明るい黄色）
	gameClearText_ = std::make_unique<TextSprite>();
	gameClearText_->Initialize();
	gameClearText_->SetAnchor({ 0.5f, 0.5f, 0.0f });
	gameClearText_->SetText(L"GameClear", 80, RGB(255, 215, 0));
	gameClearText_->SetTranslate({ 640.0f, 260.0f });

	// 「GameOver」テキストスプライトの生成（赤色）
	gameOverText_ = std::make_unique<TextSprite>();
	gameOverText_->Initialize();
	gameOverText_->SetAnchor({ 0.5f, 0.5f, 0.0f });
	gameOverText_->SetText(L"GameOver", 80, RGB(235, 60, 60));
	gameOverText_->SetTranslate({ 640.0f, 260.0f });

	// タイトルへ戻る案内「SpaceToTitle」
	spaceToTitleText_ = std::make_unique<TextSprite>();
	spaceToTitleText_->Initialize();
	spaceToTitleText_->SetAnchor({ 0.5f, 0.5f, 0.0f });
	spaceToTitleText_->SetText(L"SpaceToTitle", 36, RGB(220, 220, 220));
	spaceToTitleText_->SetTranslate({ 640.0f, 480.0f });
}

void ResultScene::Unload() {
	gameClearText_.reset();
	gameOverText_.reset();
	spaceToTitleText_.reset();
}

void ResultScene::Update(float deltaTime) {
	timer_ += deltaTime;

	// 結果に応じたテキストの更新
	if (isClear_) {
		if (gameClearText_) {
			gameClearText_->Update();
		}
	} else {
		if (gameOverText_) {
			gameOverText_->Update();
		}
	}

	if (spaceToTitleText_) {
		// 点滅演出
		float alpha = 0.5f + 0.5f * std::sin(timer_ * 4.0f);
		spaceToTitleText_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
		spaceToTitleText_->Update();
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
	if (isClear_) {
		if (gameClearText_) {
			gameClearText_->Draw();
		}
	} else {
		if (gameOverText_) {
			gameOverText_->Draw();
		}
	}

	if (spaceToTitleText_) {
		spaceToTitleText_->Draw();
	}
}

SceneType ResultScene::GetNextScene() const {
	return nextSceneType_;
}
