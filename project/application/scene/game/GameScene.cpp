#include "GameScene.h"

#include"../system/utility/random/RandomEngine.h"

using namespace Bonjin;

void GameScene::Initialize(Camera* camera)
{

	// 今のシーンと遷移後シーン(初期値は同じ)
	currentSceneType_ = SceneType::kGame;
	nextSceneType_ = SceneType::kGame;

	this->camera_ = camera;

	// スプライト共通
	spriteCommon_ = new SpriteCommon;
	spriteCommon_->Initialize();

	// スプライト
	sprite_ = Sprite();
	sprite_.Initialize("uvChecker.png");
	sprite_.Scale() = { 1.f,1.f };
	sprite_.Rotate() = { 0.f,0.f };
	sprite_.Translate() = { 640.f, 360.f};
	sprite_.Anchor() = { 0.5f,0.5f };
	sprite_.Size() = { 640.f, 360.f };

}

void GameScene::Unload() {
}

void GameScene::Update(float deltaTime) {

	sprite_.Update();

	if (Input::GetInstance()->IsTrigger(DIK_SPACE)) {
		nextSceneType_ = SceneType::kTitle;
	}
}

void GameScene::Draw() {

	spriteCommon_->PreDraw();

	sprite_.Draw();

	spriteCommon_->PostDraw();

}

void GameScene::DrawSceneImGui() {

	sprite_.DrawImGui();

}

SceneType GameScene::GetNextScene() const {
	return nextSceneType_;
}