#include "TitleScene.h"
#include "input/Input.h"
#include "input/Gamepad.h"
#include "SceneManager.h"
#include "GpuParticleManager.h"
#include <algorithm>
#include <cmath>

using namespace Bonjin;

void TitleScene::Initialize(Camera* camera) {
	currentSceneType_ = "TitleScene";
	nextSceneType_ = "TitleScene";
	phase_ = TitlePhase::kActive;
	camera_ = camera;
	timer_ = 0.0f;
	diveTimer_ = 0.0f;

	// GPUパーティクルのクリア
	GpuParticleManager::GetInstance()->Clear();

	// 1. 3D電脳ワールドの初期化
	world_ = std::make_unique<TitleWorld>();
	world_->Initialize(camera_);

	// 2. 2D UI / HUDの初期化
	ui_ = std::make_unique<TitleUI>();
	ui_->Initialize();

	// 3. ポストプロセスの初期設定（サイバーヴィネット＆微細ノイズ）
	auto sceneManager = SceneManager::GetInstance();
	sceneManager->ResetPostEffects();
	sceneManager->SetCRTShutDownProgress(0.0f);
	sceneManager->SetFullScreenVignette(true);
	sceneManager->SetFullScreenVignetteColor({ 0.0f, 0.04f, 0.16f }); // サイバーブルー調
	sceneManager->SetFullScreenVignetteScale(18.0f);
	sceneManager->SetFullScreenVignettePower(0.85f);
	sceneManager->AddPostEffect(DirectXCommon::PostEffect::kRandomNoise);
	sceneManager->SetNoiseAlpha(0.015f); // 走査線・ホログラム風の微細ノイズ

	// 初回開幕デジタルディゾルブ（真っ黒から実体化）
	TransitionDesc startDesc;
	startDesc.outType = TransitionType::kNone;
	startDesc.waitDuration = 0.0f;
	startDesc.inType = TransitionType::kDigitalDissolve;
	startDesc.inDuration = 1.4f;
	sceneManager->StartTransition(startDesc);
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

	// GPUパーティクルのクリア
	GpuParticleManager::GetInstance()->Clear();

	// ポストエフェクトのクリーンアップ
	auto sceneManager = SceneManager::GetInstance();
	sceneManager->ResetPostEffects();
	sceneManager->RemovePostEffect(DirectXCommon::PostEffect::kRadialBlur);
	sceneManager->SetRadialBlurWidth(0.0f);
	sceneManager->SetFullScreenVignette(false);
	sceneManager->SetNoiseAlpha(0.0f);
}

void TitleScene::Update(float deltaTime) {
	timer_ += deltaTime;

	bool isDiving = (phase_ == TitlePhase::kDive);
	float diveProgress = isDiving ? std::clamp(diveTimer_ / kDiveDuration, 0.0f, 1.0f) : 1.0f;

	// 1. 3D空間ワールドの更新
	if (world_) {
		world_->Update(deltaTime, isDiving, diveProgress);
	}

	// 2. GPUパーティクル（雷霆・スパーク）の更新
	GpuParticleManager::GetInstance()->Update(deltaTime, camera_);

	// 3. 2D UI / HUDの更新
	if (ui_) {
		ui_->Update(deltaTime, isDiving, diveProgress);
	}

	// 4. シーンフェーズ処理
	switch (phase_) {
	case TitlePhase::kActive:
		// スペースキーまたはゲームパッドAボタンでダイブ突入開始
		if (Input::GetInstance()->IsTrigger(DIK_SPACE) || Gamepad::GetInstance()->IsTrigger(XINPUT_GAMEPAD_A)) {
			// 選択中のメニューに応じて遷移先を決定（GameScene または TutorialScene）
			if (ui_ && ui_->GetSelectedMenuItem() == TitleUI::TitleMenuItem::kTutorial) {
				targetSceneType_ = "TutorialScene";
			} else {
				targetSceneType_ = "GameScene";
			}

			ChangePhase(TitlePhase::kDive);
			diveTimer_ = 0.0f;

			// ラジアルブラーの追加（光速突入演出）
			auto sceneManager = SceneManager::GetInstance();
			auto activeEffects = sceneManager->GetActiveEffects();
			if (std::find(activeEffects.begin(), activeEffects.end(), DirectXCommon::PostEffect::kRadialBlur) == activeEffects.end()) {
				sceneManager->AddPostEffect(DirectXCommon::PostEffect::kRadialBlur);
			}

			// スタート時のカメラ激震シェイク
			if (camera_) {
				camera_->StartShake(4.5f, 1.0f);
			}
		}
		break;

	case TitlePhase::kDive:
		UpdateDiveTransition(deltaTime);
		break;
	}
}

void TitleScene::UpdateDiveTransition(float deltaTime) {
	diveTimer_ += deltaTime;
	float progress = std::clamp(diveTimer_ / kDiveDuration, 0.0f, 1.0f);
	float eased = progress * progress * progress;

	// ラジアルブラーを加速強化（光速の吸引感）
	auto sceneManager = SceneManager::GetInstance();
	sceneManager->SetRadialBlurWidth(eased * 0.11f);

	// ズーム最高潮に達した瞬間、共通トランジション（CRT電源断 → 静寂ブラックアウト → 次シーンディゾルブ）を起動！
	if (progress >= 1.0f) {
		sceneManager->SetRadialBlurWidth(0.0f);
		sceneManager->RemovePostEffect(DirectXCommon::PostEffect::kRadialBlur);

		// ハイブリッド方式：シーン名だけ渡せば、TitleSceneのOut設定と次シーンのIn設定が自動適用される！
		sceneManager->RequestSceneChange(targetSceneType_);
	}
}

void TitleScene::Draw() {
	// 1. 3D空間ワールドの描画
	if (world_) {
		world_->Draw();
	}

	// 2. GPUパーティクル（雷霆・スパーク）の描画
	if (camera_) {
		GpuParticleManager::GetInstance()->Draw(camera_);
	}

	// 3. 2D HUDテキストの描画
	if (ui_) {
		ui_->Draw();
	}
}

void TitleScene::DrawImGui() {
#ifdef USE_IMGUI
	if (world_) {
		world_->DrawImGui();
	}
	GpuParticleManager::GetInstance()->DrawImGui();
#endif
}

SceneType TitleScene::GetNextScene() const {
	return nextSceneType_;
}