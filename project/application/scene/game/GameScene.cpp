#include "GameScene.h"

#include"../system/utility/random/RandomEngine.h"

using namespace BonjinEngine;

void GameScene::Initialize(Camera* camera) {
	// 今のシーンと遷移後シーン(初期値は同じ)
	currentSceneType_ = SceneType::kGame;
	nextSceneType_ = SceneType::kGame;

	this->camera_ = camera;

	/*particle_ = new Particle;
	particle_->LoadModel("plane");
	particle_->Emit({ 0.0f, 0.0f, 0.0f }, { 5.0f, 5.0f, 5.0f }, 50.0f, 1.0f, 3.0f);*/
}

void GameScene::Unload() {
	//if (particle_ != nullptr) {
	//	delete particle_;
	//	particle_ = nullptr;
	//}
}

void GameScene::Update(float deltaTime) {
	//particle_->Update(camera_);

	if (Input::GetInstance()->IsTrigger(DIK_SPACE)) {
		nextSceneType_ = SceneType::kTitle;
	}
}

void GameScene::Draw() {
	//particle_->Draw();
}

void GameScene::DrawSceneImGui() {
}

SceneType GameScene::GetNextScene() const {
	return nextSceneType_;
}