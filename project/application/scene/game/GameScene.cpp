#include "GameScene.h"

#include"../system/utility/random/RandomEngine.h"

using namespace Bonjin;

void GameScene::Initialize(Camera* camera)
{

	// 今のシーンと遷移後シーン(初期値は同じ)
	currentSceneType_ = SceneType::kGame;
	nextSceneType_ = SceneType::kGame;

	this->camera_ = camera;

	// スプライト
	sprite_ = Sprite();
	sprite_.Initialize("uvChecker.png");
	sprite_.Scale() = { 1.f,1.f };
	sprite_.Rotate() = { 0.f,0.f };
	sprite_.Translate() = { 0.f, 0.f};
	sprite_.Anchor() = { 0.f,0.f };
	sprite_.Size() = { 320.f, 180.f };

	//
	model_ = Model();
	model_.CreateSphere(36);

}

void GameScene::Unload() {
}

void GameScene::Update(float deltaTime) {

	sprite_.Update();

	model_.Update(InitializeWorldTransform(), camera_);

	// フェーズに応じた処理
	switch (phase_) {
	case GamePhase::kStart:
		// フェード演出など
		ChangePhase(GamePhase::kPlay);
		break;

	case GamePhase::kPlay:

		if (Input::GetInstance()->IsTrigger(DIK_SPACE)) {
			ChangePhase(GamePhase::kGoal);
		}
		break;

	case GamePhase::kGoal:

		nextSceneType_ = SceneType::kGame;

		break;
	}
}

void GameScene::Draw() {

	sprite_.Draw();

	model_.Draw();

}

void GameScene::DrawSceneImGui() {
	
}

SceneType GameScene::GetNextScene() const {
	return nextSceneType_;
}