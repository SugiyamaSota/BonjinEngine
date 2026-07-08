#include "TestScene.h"

#include <cmath>

using namespace Bonjin;

void TestScene::Initialize(Camera* camera)
{

	// 今のシーンと遷移後シーン(初期値は同じ)
	currentSceneType_ = "TestScene";
	nextSceneType_ = "TestScene";

	this->camera_ = camera;

	testModel_ = std::make_unique<Object3D>();
	testModel_->CreateModel(ModelBuilder::ModelType::kSphere, "resources/textures/uvChecker.png");
	testModel_->SetName("Sphere (testModel_)");
	//testModel_->LoadModel("teapot", "teapot.obj");


	testSkyBox_ = std::make_unique<SkyBox>();
	testSkyBox_->CreateModel(ModelBuilder::ModelType::kSkyBox, "resources/textures/skyBox.dds");
	testSkyBox_->SetName("SkyBox (testSkyBox_)");

	testCube_ = std::make_unique<Object3D>();
	testCube_->LoadModel("human", "walk.gltf");
	testCube_->SetName("Human (testCube_)");
	testCube_->SetEnableEnableEnvironmentMap(false);
	testCube_->SetCullMode(D3D12_CULL_MODE_FRONT);
	cubeAnimation_ = AnimationBuilder::LoadAnimationFile("resources/models/human", "walk.gltf");
	skeletonDebugRenderer_ = std::make_unique<SkeletonDebugRenderer>();
	if (testCube_->GetSkeleton().has_value()) {
		skeletonDebugRenderer_->Initialize(testCube_->GetSkeleton()->joints.size());
	}
	animatedModelWorldMatrix_ = MakeIdentity4x4();

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

	if (testCube_->GetSkeleton().has_value()) {
		SkeletonBuilder::ApplyAnimation(*testCube_->GetSkeleton(), cubeAnimation_, animationTime_);
	}
	animatedModelWorldMatrix_ = MakeAffineMatrix(
		Vector3{ 1.0f, 1.0f, 1.0f },
		Vector3{ 0.0f, 3.14159265f, 0.0f },
		Vector3{ 2.5f, 0.0f, 0.0f });
	testCube_->Update(animatedModelWorldMatrix_, camera_);
	if (testCube_->GetSkeleton().has_value()) {
		skeletonDebugRenderer_->Update(*testCube_->GetSkeleton(), animatedModelWorldMatrix_, camera_);
	}

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

		nextSceneType_ = "TestScene";

		break;
	}
}

void TestScene::Draw() {

	testModel_->Draw();

	testSkyBox_->Draw();

	testCube_->Draw();

	testSprite_->Draw();

	pm->Draw();

	skeletonDebugRenderer_->Draw();
}

void TestScene::DrawSceneImGui() {
#ifdef USE_IMGUI

	LightManager::GetInstance()->DrawImGui();

#endif
}

SceneType TestScene::GetNextScene() const {
	return nextSceneType_;
}
