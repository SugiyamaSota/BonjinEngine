#include "TestScene.h"

#include <cmath>

using namespace Bonjin;

void TestScene::Initialize(Camera* camera)
{

	// 今のシーンと遷移後シーン(初期値は同じ)
	currentSceneType_ = SceneType::kTest;
	nextSceneType_ = SceneType::kTest;

	this->camera_ = camera;

	testModel_ = std::make_unique<Object3D>();
	testModel_->CreateModel(ModelBuilder::ModelType::kSphere, "resources/textures/uvChecker.png");
	//testModel_->LoadModel("teapot", "teapot.obj");


	testSkyBox_ = std::make_unique<SkyBox>();
	testSkyBox_->CreateModel(ModelBuilder::ModelType::kSkyBox, "resources/textures/skyBox.dds");

	testCube_ = std::make_unique<Object3D>();
	testCube_->LoadModel("animatedCube", "AnimatedCube.gltf");
	testCube_->SetEnableEnableEnvironmentMap(false);
	cubeAnimation_ = AnimationBuilder::LoadAnimationFile("resources/models/animatedCube", "AnimatedCube.gltf");

	testSprite_ = std::make_unique<Sprite>();
	testSprite_->Initialize("uvChecker.png");

	pm = ParticleManager::GetInstance();
	pm->Initialize();

	// 「炎」と「火花」など、見た目（モデルやテクスチャ）ごとにグループを作る
	pm->CreateParticleGroup("ringGroup", ModelBuilder::ModelType::kCylinder, "resources/textures/gradationLine.png");

	ringEffect.position = { 0.0f, -1.0f, 0.0f };
	ringEffect.rotate = { 0.0f, 0.0f, 0.0f };
	ringEffect.velocity = { 0.0f, 0.0f, 0.0f };
	ringEffect.scale = { 1.0f, 1.0f, 1.0f }; // 大きなリングにする
	ringEffect.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // オレンジ色
	ringEffect.lifeTime = 2.0f;

	auto ringUpdate = [](ParticleData& data, float deltaTime) {
		data.transform.translate.y += deltaTime; // 回転速度を追
		};

	ringEffect.updateFunc = ringUpdate;

	ParticleManager::GetInstance()->Emit("ringGroup", ringEffect);

}

void TestScene::Unload() {
}

void TestScene::Update(float deltaTime) {

	testModel_->Update(InitializeWorldTransform(), camera_);

	testSkyBox_->Update(InitializeWorldTransform(), camera_);

	if (cubeAnimation_.duration > 0.0f) {
		animationTime_ += deltaTime;
		animationTime_ = std::fmod(animationTime_, cubeAnimation_.duration);
	}

	NodeAnimation& cubeAnimation = cubeAnimation_.nodeAnimations["AnimatedCube"];
	Vector3 translate = cubeAnimation.translate.keyframes.empty()
		? Vector3{ 0.0f, 0.0f, 0.0f }
		: AnimationBuilder::CalculateValue(cubeAnimation.translate.keyframes, animationTime_);
	Quaternion rotate = cubeAnimation.rotate.keyframes.empty()
		? Quaternion{ 0.0f, 0.0f, 0.0f, 1.0f }
		: AnimationBuilder::CalculateValue(cubeAnimation.rotate.keyframes, animationTime_);
	Vector3 scale = cubeAnimation.scale.keyframes.empty()
		? Vector3{ 1.0f, 1.0f, 1.0f }
		: AnimationBuilder::CalculateValue(cubeAnimation.scale.keyframes, animationTime_);
	Matrix4x4 localMatrix = MakeAffineMatrix(scale, MakeRotateMatrix(rotate), translate);
	Matrix4x4 worldMatrix = Multiply(localMatrix, MakeTranslateMatrix({ 2.5f, 0.0f, 0.0f }));
	testCube_->Update(worldMatrix, camera_);

	testSprite_->Update();

	if (Input::GetInstance()->IsTrigger(DIK_SPACE)) {
		ParticleManager::GetInstance()->Emit("ringGroup", ringEffect);
	}

	pm->Update(camera_);

	// フェーズに応じた処理
	switch (phase_) {
	case TestPhase::kStart:
		// フェード演出など
		ChangePhase(TestPhase::kPlay);
		break;

	case TestPhase::kPlay:

		if (Input::GetInstance()->IsTrigger(DIK_SPACE)) {
			ChangePhase(TestPhase::kGoal);
		}
		break;

	case TestPhase::kGoal:

		nextSceneType_ = SceneType::kTest;

		break;
	}
}

void TestScene::Draw() {

	testModel_->Draw();

	testSkyBox_->Draw();

	testCube_->Draw();

	testSprite_->Draw();

	pm->Draw();
}

void TestScene::DrawSceneImGui() {
#ifdef USE_IMGUI

	LightManager::GetInstance()->DrawImGui();


	testModel_->DrawImGui("TestModel");


#endif
}

SceneType TestScene::GetNextScene() const {
	return nextSceneType_;
}
