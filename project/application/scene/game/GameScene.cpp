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
	//sprite_ = Sprite();
	//sprite_.Initialize("uvChecker.png");
	//sprite_.Scale() = { 1.f,1.f };
	//sprite_.Rotate() = { 0.f,0.f };
	//sprite_.Translate() = { 0.f, 0.f};
	//sprite_.Anchor() = { 0.f,0.f };
	//sprite_.Size() = { 320.f, 180.f };

	//
	WT_ = InitializeWorldTransform();
	model_ = Model();
	model_.CreateSphere(36);

	terrainWT_ = InitializeWorldTransform();
	terrainModel_ = Model();
	terrainModel_.LoadModel("terrain");

}

void GameScene::Unload() {
}

void GameScene::Update(float deltaTime) {

	//sprite_.Update();

	model_.Update(WT_, camera_);
	terrainModel_.Update(WT_, camera_);

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
}

void GameScene::DrawSceneImGui() {
#ifdef USE_IMGUI
	if (ImGui::TreeNode("WorldTransform")) {
		ImGui::DragFloat3("scale", &WT_.scale.x, 0.1f);
		ImGui::DragFloat3("rotate", &WT_.rotate.x, 0.1f);
		ImGui::DragFloat3("translate", &WT_.translate.x, 0.1f);
		ImGui::TreePop();
	}
#endif
	model_.DrawImGui();
	LightManager::GetInstance()->DrawImGui();
}

SceneType GameScene::GetNextScene() const {
	return nextSceneType_;
}