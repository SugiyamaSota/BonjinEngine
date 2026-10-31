#include "GameScene.h"
#include "phase/Highlight.h"
#include "phase/Play.h"

#include "gameObject/player/PlayerStatusRepository.h"

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
	battleController_->Initialize(camera_, "resources/maps/stage1.json");

	// ハイライトデータが存在する場合は kHighlight フェーズを開始、なければ kPlay フェーズ
	const auto& highlights = battleController_->GetMapChipField()->GetHighlights();
	if (!highlights.empty()) {
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
#endif
}