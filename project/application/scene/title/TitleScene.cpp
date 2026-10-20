#include "TitleScene.h"
#include "TitlePhase.h"

#include "SceneManager.h"
#include "GpuParticleManager.h"

using namespace Bonjin;

void TitleScene::Initialize(Camera* camera) {
	currentSceneType_ = "TitleScene";
	nextSceneType_ = "TitleScene";
	camera_ = camera;
	timer_ = 0.0f;

	// 初期フェーズを設定
	ChangePhase(std::make_unique<TitlePhaseActive>());

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
	sceneManager->AddPostEffect(PostEffect::kRandomNoise);
	sceneManager->SetNoiseAlpha(0.015f); // 走査線・ホログラム風の微細ノイズ

	// 初回開幕デジタルディゾルブ（真っ黒から実体化）
	TransitionDesc startDesc;
	startDesc.outType = TransitionType::kNone;
	startDesc.waitDuration = 0.0f;
	startDesc.inType = TransitionType::kDigitalDissolve;
	startDesc.inDuration = 1.4f;
	startDesc.dissolveMode = DissolveMode::kRandom;
	sceneManager->StartTransition(startDesc);

	// BGM再生（ループ）
	BgmPlayer::GetInstance()->Play("resources/sounds/bgm/title.mp3", true);

	// SE登録
	shutdownSEId = SoundManager::GetInstance()->LoadSound("resources/sounds/se/test2.mp3");
}

void TitleScene::Unload() {
	if (currentPhase_) {
		currentPhase_->Exit(this);
		currentPhase_.reset();
	}

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
	sceneManager->RemovePostEffect(PostEffect::kRadialBlur);
	sceneManager->SetRadialBlurWidth(0.0f);
	sceneManager->SetFullScreenVignette(false);
	sceneManager->SetNoiseAlpha(0.0f);
}

void TitleScene::Update(float deltaTime) {
	timer_ += deltaTime;

	auto phase = static_cast<ITitlePhase*>(GetCurrentPhase());
	bool isDiving = phase ? phase->IsDiving() : false;
	float diveProgress = phase ? phase->GetDiveProgress() : 1.0f;

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

	// 4. シーンフェーズ処理（ステート委譲）
	if (currentPhase_) {
		currentPhase_->Update(this, deltaTime);
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

	if (currentPhase_) {
		currentPhase_->Draw(this);
	}
}

void TitleScene::DrawImGui() {
#ifdef USE_IMGUI
	if (world_) {
		world_->DrawImGui();
	}
	GpuParticleManager::GetInstance()->DrawImGui();

	if (currentPhase_) {
		currentPhase_->DrawImGui(this);
	}
#endif
}

SceneType TitleScene::GetNextScene() const {
	return nextSceneType_;
}