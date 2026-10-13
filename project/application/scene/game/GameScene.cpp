#include "GameScene.h"

#include "gameObject/player/PlayerStatusRepository.h"
#include "../result/ResultScene.h"
#include "SceneManager.h"
#include <algorithm>

using namespace Bonjin;

void GameScene::Initialize(Camera* camera)
{
	camera_ = camera;
	phase_ = GamePhase::kStart;
	startTransitionTimer_ = 0.0f;

	// 今のシーンと遷移後シーン(初期値は同じ)
	currentSceneType_ = "GameScene";
	nextSceneType_ = "GameScene";

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

}

void GameScene::Unload() {
	if (battleController_) {
		battleController_->Unload();
		battleController_.reset();
	}

	auto sceneManager = SceneManager::GetInstance();
	sceneManager->ResetPostEffects();
	sceneManager->RemovePostEffect(DirectXCommon::PostEffect::kDigitalDissolve);
}

void GameScene::Update(float deltaTime) {
	battleController_->Update(deltaTime);

	if (battleController_->IsGoalReached()) {
		ResultScene::SetIsClear(true);
		ChangePhase(GamePhase::kGoal);
	} else if (battleController_->GetPlayer() && battleController_->GetPlayer()->GetStatus().IsDead()) {
		ResultScene::SetIsClear(false);
		ChangePhase(GamePhase::kGoal);
	}

	switch (phase_) {
	case GamePhase::kStart: {
		// 開幕のデジタルディゾルブ（真っ黒から電子ブロック実体化）
		startTransitionTimer_ += deltaTime;
		float progress = std::clamp(startTransitionTimer_ / kStartTransitionDuration, 0.0f, 1.0f);
		auto sceneManager = SceneManager::GetInstance();
		sceneManager->SetDissolveThreshold(progress);

		if (progress >= 1.0f) {
			sceneManager->RemovePostEffect(DirectXCommon::PostEffect::kDigitalDissolve);
			ChangePhase(GamePhase::kPlay);
		}
		break;
	}
	case GamePhase::kPlay:
		
		break;
	case GamePhase::kGoal:

		nextSceneType_ = "ResultScene";

		break;
	}
}

void GameScene::Draw() {
	battleController_->Draw();
}

SceneType GameScene::GetNextScene() const {
	return nextSceneType_;
}

void GameScene::DrawSceneImGui() {
#ifdef USE_IMGUI
	if (battleController_) {
		battleController_->DrawImGui();
	}
#endif
}