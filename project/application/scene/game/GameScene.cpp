#include "GameScene.h"
#include "phase/Highlight.h"
#include "phase/Play.h"

#include "gameObject/player/PlayerStatusRepository.h"
#include "../../demo/DemoManager.h"

using namespace Bonjin;

void GameScene::Initialize(Camera* camera)
{
	camera_ = camera;

	// 今のシーンと遷移後シーン(初期値は同じ)
	currentSceneType_ = "GameScene";
	nextSceneType_ = "GameScene";

	// プレイヤーステータスの読み込み
	PlayerStatusRepository::GetInstance()->Load("resources/data/player_status.json");

	battleController_ = std::make_unique<BattleController>();
	// デモモードのときはデモ用のステージにする
	const bool isDemoMode = DemoManager::GetInstance()->IsDemoMode();
	battleController_->Initialize(camera_, isDemoMode ? "resources/maps/demo_stage.json" : "resources/maps/stage1.json");

	// デモモードなら、プレイヤーへ命令を出す側を人の入力からデモのAIに差し替える
	if (isDemoMode) {
		battleController_->SetPlayerController(
			std::make_unique<DemoPlayerController>(DemoManager::GetInstance()->GetAutoPlaySettings()));
	}
	DemoManager::GetInstance()->BeginGame();

	// ハイライトデータが存在する場合は kHighlight フェーズを開始、なければ kPlay フェーズ
	const auto& highlights = battleController_->GetMapChipField()->GetHighlights();
	if (!highlights.empty() && !isDemoMode) {
		ChangePhase(std::make_unique<GamePhase::Highlight>());
	} else {
		ChangePhase(std::make_unique<GamePhase::Play>());
	}
}

void GameScene::Unload() {
	if (currentPhase_) {
		currentPhase_->Exit(this);
		currentPhase_.reset();
	}
	if (battleController_) {
		battleController_->Unload();
		battleController_.reset();
	}
	// 本物の入力に戻す
	DemoManager::GetInstance()->EndGame();
}

void GameScene::Update(float deltaTime) {
	if (currentPhase_) {
		currentPhase_->Update(this, deltaTime);
	}
}

void GameScene::Draw() {
	if (battleController_) {
		battleController_->Draw();
	}
	if (currentPhase_) {
		currentPhase_->Draw(this);
	}
}

SceneType GameScene::GetNextScene() const {
	return nextSceneType_;
}

void GameScene::DrawSceneImGui() {
#ifdef USE_IMGUI
	if (currentPhase_) {
		currentPhase_->DrawImGui(this);
	}

	if (battleController_) {
		battleController_->DrawImGui();
	}

	DemoManager::GetInstance()->DrawImGui();
#endif
}