#include "TitleScene.h"

using namespace BonjinEngine;

void TitleScene::Initialize(Camera* camera) {
	currentSceneType_ = SceneType::kTitle;
	nextSceneType_ = SceneType::kTitle;

	this->camera_ = camera;

	worldTransform_ = InitializeWorldTransform();
	worldTransform_.translate = { 640.f,360.f,0.f };
	sprite_ = new Sprite();
	sprite_->Initialize(worldTransform_, Color::White, { 0.5f,0.5f }, { 512.f,512.f }, "uvChecker.png", { 0.f,0.f }, { 256.f,256.f });
}

void TitleScene::Unload() {
	if (sprite_ != nullptr) {
		delete sprite_;
		sprite_ = nullptr;
	}
}

void TitleScene::Update(float deltaTime) {

	//worldTransform_.rotate.z += 0.1f;
	sprite_->Update(worldTransform_, Color::White);

	if (Input::GetInstance()->IsTrigger(DIK_SPACE)) {
		nextSceneType_ = SceneType::kGame;
	}
}

void TitleScene::Draw() {
	sprite_->Draw();
}

void TitleScene::DrawSceneImGui() {

	sprite_->DrawImGui();

}

SceneType TitleScene::GetNextScene() const {
	return nextSceneType_;
}