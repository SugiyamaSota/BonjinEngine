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
	testModel_->CreateModel(ModelBuilder::ModelType::kSphere, "resources/textures/uvChecker.png");
	//testModel_->LoadModel("teapot", "teapot.obj");


	testSkyBox_ = std::make_unique<Object3D>();
	testSkyBox_->CreateModel(ModelBuilder::ModelType::kCube, "resources/textures/skyBox.dds");
	testSkyBox_->SetPrimitiveType(PrimitiveType::kSkyBox);

	pm = ParticleManager::GetInstance();
	pm->Initialize();

	// 「炎」と「火花」など、見た目（モデルやテクスチャ）ごとにグループを作る
	pm->CreateParticleGroup("ringGroup", ModelBuilder::ModelType::kRing, "resources/textures/gradationLine.png");


	ringEffect.position = { 0.0f, 0.0f, 0.0f };
	ringEffect.rotate = { 0.0f, 0.0f, 0.0f };
	ringEffect.velocity = { 0.0f, 0.0f, 0.0f };
	ringEffect.scale = { 1.0f, 1.0f, 1.0f }; // 大きなリングにする
	ringEffect.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // オレンジ色
	ringEffect.lifeTime = 2.0f;

	auto ringUpdate = [](ParticleData& data, float deltaTime) {
		data.transform.rotate.z += deltaTime; // 回転速度を追
		};

	ringEffect.updateFunc = ringUpdate;

	ParticleManager::GetInstance()->Emit("ringGroup", ringEffect);

}

void GameScene::Unload() {
}

void GameScene::Update(float deltaTime) {

	testModel_->Update(InitializeWorldTransform(), camera_);

	testSkyBox_->Update(InitializeWorldTransform(), camera_);

	if (Input::GetInstance()->IsTrigger(DIK_SPACE)) {
		ParticleManager::GetInstance()->Emit("ringGroup", ringEffect);
	}

	pm->Update(camera_);

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

	//testModel_->Draw();

	//testSkyBox_->Draw();

	pm->Draw();
}

void GameScene::DrawSceneImGui() {
#ifdef USE_IMGUI

	LightManager::GetInstance()->DrawImGui();

#endif
}

SceneType GameScene::GetNextScene() const {
	return nextSceneType_;
}