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
	phase_ = TitlePhase::kFadeIn;
	camera_ = camera;
	timer_ = 0.0f;
	fadeInTimer_ = 0.0f;
	diveTimer_ = 0.0f;
	shutDownTimer_ = 0.0f;
	blackoutTimer_ = 0.0f;

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

	// 開幕デジタルディゾルブ（完全な真っ黒から電子ブロック実体化）
	sceneManager->AddPostEffect(DirectXCommon::PostEffect::kDigitalDissolve);
	sceneManager->SetDissolveThreshold(0.0f); // 真っ黒からスタート
	sceneManager->SetDissolveEdgeWidth(0.06f);
	sceneManager->SetDissolveEdgeColor({ 0.0f, 1.5f, 2.0f }); // サイバーシアン
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

	// ポストエフェクトのクリーンアップ（次シーンへの影響を防止）
	auto sceneManager = SceneManager::GetInstance();
	sceneManager->ResetPostEffects();
	sceneManager->SetCRTShutDownProgress(0.0f);
	sceneManager->RemovePostEffect(DirectXCommon::PostEffect::kDigitalDissolve);
	sceneManager->RemovePostEffect(DirectXCommon::PostEffect::kCRTShutDown);
	sceneManager->RemovePostEffect(DirectXCommon::PostEffect::kRadialBlur);
	sceneManager->SetFullScreenVignette(false);
	sceneManager->SetRadialBlurWidth(0.0f);
	sceneManager->SetNoiseAlpha(0.0f);
}

void TitleScene::Update(float deltaTime) {
	timer_ += deltaTime;

	// ブラックアウトフェーズ中は3D空間・パーティクル・UIの更新を全停止して完全な静寂を演出
	if (phase_ == TitlePhase::kBlackout) {
		UpdateBlackout(deltaTime);
		return;
	}

	bool isDiving = (phase_ == TitlePhase::kDive || phase_ == TitlePhase::kShutDown);
	float diveProgress = (phase_ == TitlePhase::kDive) ? std::clamp(diveTimer_ / kDiveDuration, 0.0f, 1.0f) : 1.0f;

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
	case TitlePhase::kFadeIn: {
		// 開幕のデジタルディゾルブ（真っ黒から電子ブロック実体化）
		fadeInTimer_ += deltaTime;
		float progress = std::clamp(fadeInTimer_ / kFadeInDuration, 0.0f, 1.0f);
		auto sceneManager = SceneManager::GetInstance();
		sceneManager->SetDissolveThreshold(progress);

		if (progress >= 1.0f) {
			sceneManager->RemovePostEffect(DirectXCommon::PostEffect::kDigitalDissolve);
			ChangePhase(TitlePhase::kActive);
		}
		break;
	}

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

	case TitlePhase::kShutDown:
		UpdateShutDownTransition(deltaTime);
		break;

	case TitlePhase::kBlackout:
		// 上部で処理済み
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

	// ズーム最高潮に達した瞬間、ポストエフェクトによる「プツン」電源断演出へ突入！
	if (progress >= 1.0f) {
		ChangePhase(TitlePhase::kShutDown);
		shutDownTimer_ = 0.0f;

		// ラジアルブラーを解除し、CRTシャットダウンポストエフェクトを追加！
		sceneManager->SetRadialBlurWidth(0.0f);
		sceneManager->RemovePostEffect(DirectXCommon::PostEffect::kRadialBlur);

		auto activeEffects = sceneManager->GetActiveEffects();
		if (std::find(activeEffects.begin(), activeEffects.end(), DirectXCommon::PostEffect::kCRTShutDown) == activeEffects.end()) {
			sceneManager->AddPostEffect(DirectXCommon::PostEffect::kCRTShutDown);
		}
		sceneManager->SetCRTShutDownProgress(0.0f);
	}
}

void TitleScene::UpdateShutDownTransition(float deltaTime) {
	shutDownTimer_ += deltaTime;
	float progress = std::clamp(shutDownTimer_ / kShutDownDuration, 0.0f, 1.0f);

	// シェーダーに進行度を渡す（画面の映像が上下から中央へ押し潰され、ホワイトラインを経て点収束消失）
	auto sceneManager = SceneManager::GetInstance();
	sceneManager->SetCRTShutDownProgress(progress);

	// プツン演出完了 → 完全ブラックアウトへ移行！
	if (progress >= 1.0f) {
		sceneManager->SetCRTShutDownProgress(1.0f); // 完全ブラックアウト
		ChangePhase(TitlePhase::kBlackout);
		blackoutTimer_ = 0.0f;
	}
}

void TitleScene::UpdateBlackout(float deltaTime) {
	blackoutTimer_ += deltaTime;

	// パチンコ特有の「息を呑む静寂のタメ（ブラックアウト）」の後に選択された次シーンへ突入！
	if (blackoutTimer_ >= kBlackoutDuration) {
		nextSceneType_ = targetSceneType_;
	}
}

void TitleScene::Draw() {
	// ブラックアウトフェーズ中は画面全体が完全な漆黒になるため3D描画等は省略
	if (phase_ == TitlePhase::kBlackout) {
		return;
	}

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
	if (phase_ != TitlePhase::kBlackout) {
		if (world_) {
			world_->DrawImGui();
		}
		GpuParticleManager::GetInstance()->DrawImGui();
	}
#endif
}

SceneType TitleScene::GetNextScene() const {
	return nextSceneType_;
}