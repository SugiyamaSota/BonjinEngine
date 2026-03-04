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
	sprite_.Translate() = { 0.f, 0.f };
	sprite_.Anchor() = { 0.f,0.f };
	sprite_.Size() = { 320.f, 180.f };
	sprite_.SetTextureRect(0, 0, 600.f, 600.f);

	//
	WT_ = InitializeWorldTransform();
	model_ = Model();
	model_.CreateSphere(36);

	terrainWT_ = InitializeWorldTransform();
	terrainModel_ = Model();
	terrainModel_.LoadModel("terrain");
	terrainModel_.SetCullMode(D3D12_CULL_MODE_BACK);

	particleMgr_ = ParticleManager::GetInstance();
	particleMgr_->CreateParticleGroup("Fire", "plane");
	particleMgr_->Emit("Fire", { 0.f,0.f,0.f }, { 1.0f, 1.0f, 1.0f }, 0.5f, 1.0f, 2.0f);

}

void GameScene::Unload() {
	ParticleManager::GetInstance()->Finalize();
}

void GameScene::Update(float deltaTime) {



	model_.Update(WT_, camera_);
	terrainModel_.Update(terrainWT_, camera_);
	sprite_.Update();
	particleMgr_->Update(camera_);

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

	model_.Draw();
	terrainModel_.Draw();
	sprite_.Draw();
	particleMgr_->Draw();
}

void GameScene::DrawSceneImGui() {
	model_.DrawImGui();
	LightManager::GetInstance()->DrawImGui();
}

SceneType GameScene::GetNextScene() const {
	return nextSceneType_;
}