#include "TutorialScene.h"
#include "../game/gameObject/player/PlayerStatusRepository.h"
#include "SceneManager.h"
#include <algorithm>

using namespace Bonjin;

void TutorialScene::Initialize(Camera* camera) {
	currentSceneType_ = "TutorialScene";
	nextSceneType_ = currentSceneType_;
	phase_ = TutorialPhase::kStart;
	camera_ = camera;
	startTransitionTimer_ = 0.0f;

	// 開幕デジタルディゾルブ（完全な真っ黒から電子ブロック実体化）の初期設定
	auto sceneManager = SceneManager::GetInstance();
	sceneManager->ResetPostEffects();
	sceneManager->AddPostEffect(DirectXCommon::PostEffect::kDigitalDissolve);
	sceneManager->SetDissolveThreshold(0.0f); // 真っ黒からスタート
	sceneManager->SetDissolveEdgeWidth(0.06f);
	sceneManager->SetDissolveEdgeColor({ 0.0f, 1.5f, 2.0f }); // サイバーシアン

	// プレイヤーステータスの読み込み
	PlayerStatusRepository::GetInstance()->Load("resources/data/player_status.json");

	battleController_ = std::make_unique<BattleController>();
	battleController_->Initialize(camera_, "resources/maps/tutorial.csv");

	tutorialManager_ = std::make_unique<TutorialManager>();
	tutorialManager_->Initialize("resources/data/tutorial.json");

	tutorialEditor_ = std::make_unique<TutorialEditor>();
	tutorialEditor_->Initialize(tutorialManager_.get());
}

void TutorialScene::Unload() {
	if (tutorialEditor_) {
		tutorialEditor_.reset();
	}
	if (tutorialManager_) {
		tutorialManager_.reset();
	}
	if (battleController_) {
		battleController_->Unload();
		battleController_.reset();
	}

	auto sceneManager = SceneManager::GetInstance();
	sceneManager->ResetPostEffects();
	sceneManager->RemovePostEffect(DirectXCommon::PostEffect::kDigitalDissolve);
}

void TutorialScene::Update(float deltaTime) {
	battleController_->Update(deltaTime);

	if (tutorialEditor_) {
		tutorialEditor_->Update(deltaTime);
	}

	if (tutorialManager_) {
		tutorialManager_->Update(battleController_.get(), deltaTime);
		if (tutorialManager_->IsCompleted()) {
			ChangePhase(TutorialPhase::kComplete);
		}
	}

	switch (phase_) {
	case TutorialPhase::kStart: {
		// 開幕のデジタルディゾルブ（真っ黒から電子ブロック実体化）
		startTransitionTimer_ += deltaTime;
		float progress = std::clamp(startTransitionTimer_ / kStartTransitionDuration, 0.0f, 1.0f);
		auto sceneManager = SceneManager::GetInstance();
		sceneManager->SetDissolveThreshold(progress);

		if (progress >= 1.0f) {
			sceneManager->RemovePostEffect(DirectXCommon::PostEffect::kDigitalDissolve);
			ChangePhase(TutorialPhase::kPlay);
		}
		break;
	}
	case TutorialPhase::kPlay:
		break;
	case TutorialPhase::kComplete:
		nextSceneType_ = "GameScene";
		break;
	}
}

void TutorialScene::Draw() {
	battleController_->Draw();
	if (tutorialManager_) {
		tutorialManager_->Draw();
	}
}

SceneType TutorialScene::GetNextScene() const {
	return nextSceneType_;
}

void TutorialScene::DrawSceneImGui() {
#ifdef USE_IMGUI
	if (battleController_) {
		battleController_->DrawImGui();
	}
	if (tutorialEditor_) {
		tutorialEditor_->DrawImGui();
	} else if (tutorialManager_) {
		tutorialManager_->DrawImGui();
	}
#endif
}
