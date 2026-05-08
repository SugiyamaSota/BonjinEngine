#include "GameScene.h"

#include"../system/utility/random/RandomEngine.h"

using namespace Bonjin;

void GameScene::Initialize(Camera* camera)
{

	// 今のシーンと遷移後シーン(初期値は同じ)
	currentSceneType_ = SceneType::kGame;
	nextSceneType_ = SceneType::kGame;

	this->camera_ = camera;

	testModel_ = std::make_unique<Object3D>();
	testModel_->CreateSphere(12);

	testSkyBox_ = std::make_unique<Object3D>();
	testSkyBox_->CreateCube();
	testSkyBox_->SetPrimitiveType(PrimitiveType::kSkyBox);

}

void GameScene::Unload() {
}

void GameScene::Update(float deltaTime) {

	testModel_->Update(InitializeWorldTransform(), camera_);

	testSkyBox_->Update(InitializeWorldTransform(), camera_);

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

	testModel_->Draw();

	testSkyBox_->Draw();

}

void GameScene::DrawSceneImGui() {
#ifdef USE_IMGUI

	LightManager::GetInstance()->DrawImGui();

#endif
}

SceneType GameScene::GetNextScene() const {
	return nextSceneType_;
}