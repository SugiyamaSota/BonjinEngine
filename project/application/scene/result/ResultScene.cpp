#include "ResultScene.h"
#include "input/Input.h"
#include "input/Gamepad.h"
#include "SceneManager.h"
#include <cmath>

using namespace Bonjin;

void ResultScene::Initialize(Camera* camera) {
	currentSceneType_ = "ResultScene";
	nextSceneType_ = currentSceneType_;
	phase_ = ResultPhase::kFadeIn;
	camera_ = camera;
	timer_ = 0.0f;

	// 前シーンのポストエフェクト（ビネット等）を残さない
	SceneManager::GetInstance()->SetFullScreenVignette(false);

	// 全画面の黒背景
	background_ = std::make_unique<Sprite>();
	background_->Initialize("default.png");
	background_->SetSize({ 1280.0f, 720.0f });
	background_->SetTranslate({ 0.0f, 0.0f });
	background_->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });

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
	background_.reset();
	gameClearText_.reset();
	gameOverText_.reset();
	spaceToTitleText_.reset();
}

void ResultScene::Update(float deltaTime) {
	timer_ += deltaTime;

	if (background_) {
		background_->Update();
	}

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
		if (Input::GetInstance()->IsTrigger(DIK_SPACE) || Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_A)) {
			ChangePhase(ResultPhase::kFadeOut);
			SceneManager::GetInstance()->RequestSceneChange("TitleScene");
		}
		break;
	case ResultPhase::kFadeOut:
		break;
	}
}

void ResultScene::Draw() {
	if (background_) {
		background_->Draw();
	}

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
