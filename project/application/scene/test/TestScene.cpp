#include "TestScene.h"
#include "SceneManager.h"

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

	damageTimer_ = 0.0f;
	isLowHP_ = false;
	isPaused_ = false;
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

	// ポストエフェクトデモの更新
	if (damageTimer_ > 0.0f) {
		damageTimer_ -= deltaTime;
		if (damageTimer_ < 0.0f) damageTimer_ = 0.0f;
	}

	auto sceneManager = SceneManager::GetInstance();
	sceneManager->ClearPostEffects();

	if (isPaused_) {
		// ポーズ中はガウシアンブラーとHSVフィルター（彩度減、輝度減）
		sceneManager->AddPostEffect(DirectXCommon::PostEffect::kGaussianFilter);
		sceneManager->AddPostEffect(DirectXCommon::PostEffect::kHSVFilter);
		sceneManager->SetHSVSaturationMultiplier(0.3f);
		sceneManager->SetHSVValueMultiplier(0.5f);
	}
	else if (isLowHP_) {
		// 瀕死時はモノクロビネット＋ノイズ
		sceneManager->AddPostEffect(DirectXCommon::PostEffect::kFullScreen);
		sceneManager->SetFullScreenGray(true);
		sceneManager->SetFullScreenVignette(true);
		sceneManager->AddPostEffect(DirectXCommon::PostEffect::kRandomNoise);
		sceneManager->SetNoiseAlpha(0.3f);
	}
	else if (damageTimer_ > 0.0f) {
		// 被弾時はRadialBlurと強いノイズ（タイマーで減衰）
		float progress = damageTimer_ / 0.3f;
		sceneManager->AddPostEffect(DirectXCommon::PostEffect::kRadialBlur);
		sceneManager->SetRadialBlurWidth(progress * 0.04f);
		sceneManager->SetRadialBlurCenter({0.5f, 0.5f});
		sceneManager->AddPostEffect(DirectXCommon::PostEffect::kRandomNoise);
		sceneManager->SetNoiseAlpha(progress * 0.7f);
	}
	else {
		// 何も適用しない場合はFullScreenのデフォルト
		sceneManager->AddPostEffect(DirectXCommon::PostEffect::kFullScreen);
		sceneManager->SetFullScreenGray(false);
		sceneManager->SetFullScreenVignette(false);
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

	ImGui::Separator();
	ImGui::Text("Game Post-Effect Presets:");
	
	if (ImGui::Button("Trigger Damage Effect (0.3s)")) {
		damageTimer_ = 0.3f;
		isLowHP_ = false;
		isPaused_ = false;
	}

	if (ImGui::Checkbox("Low HP (Low Health) Effect", &isLowHP_)) {
		if (isLowHP_) {
			isPaused_ = false;
			damageTimer_ = 0.0f;
		}
	}

	if (ImGui::Checkbox("Pause (Menu Background) Blur", &isPaused_)) {
		if (isPaused_) {
			isLowHP_ = false;
			damageTimer_ = 0.0f;
		}
	}

	ImGui::Separator();
	ImGui::Text("Gamepad Test (XInput):");
	auto input = Input::GetInstance();
	if (input->IsPadConnected()) {
		ImGui::Text("Status: Connected");
		ImGui::Text("Stick L: (%ld, %ld)", input->GetPadLStickX(), input->GetPadLStickY());
		ImGui::Text("Stick R: (%ld, %ld)", input->GetPadRStickX(), input->GetPadRStickY());
		ImGui::Text("Buttons: A:%d B:%d X:%d Y:%d",
			input->IsPadPress(XINPUT_GAMEPAD_A),
			input->IsPadPress(XINPUT_GAMEPAD_B),
			input->IsPadPress(XINPUT_GAMEPAD_X),
			input->IsPadPress(XINPUT_GAMEPAD_Y));
		ImGui::Text("DPad POV: 0x%X", input->GetPadPov());
	} else {
		ImGui::Text("Status: Disconnected");
	}
#endif
}

SceneType TestScene::GetNextScene() const {
	return nextSceneType_;
}
