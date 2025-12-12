#include "TitleScene.h"

using namespace Bonjin;

void TitleScene::Initialize(Camera* camera) {
	currentSceneType_ = SceneType::kTitle;
	nextSceneType_ = SceneType::kTitle;

	this->camera_ = camera;

	
}

void TitleScene::Unload() {
	
}

void TitleScene::Update(float deltaTime) {

	

	if (Input::GetInstance()->IsTrigger(DIK_SPACE)) {
		nextSceneType_ = SceneType::kGame;
	}
}

void TitleScene::Draw() {
}

void TitleScene::DrawSceneImGui() {


}

SceneType TitleScene::GetNextScene() const {
	return nextSceneType_;
}