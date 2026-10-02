#include "TitleScene.h"
#include "input/Input.h"
#include "input/Gamepad.h"
#include "SceneManager.h"
#include <algorithm>

using namespace Bonjin;

void TitleScene::Initialize(Camera* camera) {
	currentSceneType_ = "TitleScene";
	nextSceneType_ = "TitleScene";
	phase_ = TitlePhase::kFadeIn;
	camera_ = camera;
	timer_ = 0.0f;
	diveTimer_ = 0.0f;

	// 1. 3D電脳ワールドの初期化
	world_ = std::make_unique<TitleWorld>();
	world_->Initialize(camera_);

	// 2. 2D UI / HUDの初期化
	ui_ = std::make_unique<TitleUI>();
	ui_->Initialize();

	// 3. ポストプロセスの初期設定（サイバーヴィネット＆微細ノイズ）
	auto sceneManager = SceneManager::GetInstance();
	sceneManager->ResetPostEffects();
	sceneManager->SetFullScreenVignette(true);
	sceneManager->SetFullScreenVignetteColor({ 0.0f, 0.04f, 0.16f }); // サイバーブルー調
	sceneManager->SetFullScreenVignetteScale(18.0f);
	sceneManager->SetFullScreenVignettePower(0.85f);
	sceneManager->AddPostEffect(DirectXCommon::PostEffect::kRandomNoise);
	sceneManager->SetNoiseAlpha(0.015f); // 走査線・ホログラム風の微細ノイズ
}

void TitleScene::Unload() {
	if (world_) {
		world_->Unload();
		world_.reset();
	}

	if (ui_) {
		ui_->Unload();
		ui_.reset();
	}

	// ポストエフェクトのクリーンアップ（次シーンへの影響を防止）
	auto sceneManager = SceneManager::GetInstance();
	sceneManager->ResetPostEffects();
	sceneManager->SetFullScreenVignette(false);
	sceneManager->SetRadialBlurWidth(0.0f);
	sceneManager->SetNoiseAlpha(0.0f);
}

void TitleScene::Update(float deltaTime) {
	timer_ += deltaTime;

	bool isDiving = (phase_ == TitlePhase::kFadeOut);
	float diveProgress = isDiving ? std::clamp(diveTimer_ / kDiveDuration, 0.0f, 1.0f) : 0.0f;

	// 1. 3D空間ワールドの更新
	if (world_) {
		world_->Update(deltaTime, isDiving, diveProgress);
	}

	// 2. 2D UI / HUDの更新
	if (ui_) {
		ui_->Update(deltaTime, isDiving, diveProgress);
	}

	// 3. シーンフェーズ処理
	switch (phase_) {
	case TitlePhase::kFadeIn:
		ChangePhase(TitlePhase::kActive);
		break;

	case TitlePhase::kActive:
		// スペースキーまたはゲームパッドAボタンでダイブ開始
		if (Input::GetInstance()->IsTrigger(DIK_SPACE) || Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_A)) {
			ChangePhase(TitlePhase::kFadeOut);
			diveTimer_ = 0.0f;

			// ラジアルブラーの追加
			auto sceneManager = SceneManager::GetInstance();
			auto activeEffects = sceneManager->GetActiveEffects();
			if (std::find(activeEffects.begin(), activeEffects.end(), DirectXCommon::PostEffect::kRadialBlur) == activeEffects.end()) {
				sceneManager->AddPostEffect(DirectXCommon::PostEffect::kRadialBlur);
			}

			// スタート時のカメラシェイク
			if (camera_) {
				camera_->StartShake(3.5f, 1.2f);
			}
		}
		break;

	case TitlePhase::kFadeOut:
		UpdateDiveTransition(deltaTime);
		break;
	}
}

void TitleScene::UpdateDiveTransition(float deltaTime) {
	diveTimer_ += deltaTime;
	float progress = std::clamp(diveTimer_ / kDiveDuration, 0.0f, 1.0f);
	float eased = progress * progress * progress;

	// ラジアルブラーの強化（光速突入感）
	auto sceneManager = SceneManager::GetInstance();
	sceneManager->SetRadialBlurWidth(eased * 0.09f);

	// ダイブ完了時にゲーム本編シーンへ遷移
	if (progress >= 1.0f) {
		nextSceneType_ = "GameScene";
	}
}

void TitleScene::Draw() {
	// 1. 3D空間ワールドの描画
	if (world_) {
		world_->Draw();
	}

	// 2. 2D HUDテキストの描画
	if (ui_) {
		ui_->Draw();
	}
}

SceneType TitleScene::GetNextScene() const {
	return nextSceneType_;
}