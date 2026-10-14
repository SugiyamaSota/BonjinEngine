#include "Core.h"
#include "BonjinEngine.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "GameScene.h"
#include "../scene/tutorial/TutorialScene.h"
#include "../scene/result/ResultScene.h"
#include "../scene/game/gameObject/player/PlayerStatusRepository.h"
#include "TestScene.h"

#include "ParticleManager.h"

using namespace Bonjin;

Core::Core() {}
Core::~Core() {}

void Core::Initialize() {
	Bonjin::Initialize(); // エンジン本体の初期化

	// 事前にリソースのロード
	auto modelManager = ModelManager::GetInstance();
	modelManager->LoadModel("human", "resources/models/human", "walk.gltf");

	// 共通テクスチャの事前ロード（起動時にGPU転送を完了させ、シーン遷移時の待機をゼロにする）
	auto textureManager = TextureManager::GetInstance();
	textureManager->LoadTexture("resources/textures/cube.jpg");
	textureManager->LoadTexture("resources/textures/default.png");
	textureManager->LoadTexture("resources/textures/SkyBox_Dark.dds");
	textureManager->LoadTexture("resources/textures/skyBox.dds");
	textureManager->LoadTexture("resources/textures/gradationLine.png");
	textureManager->LoadTexture("resources/textures/circle.png");
	textureManager->LoadTexture("resources/textures/noise0.png");

	// 共通パーティクルグループの事前作成（シーン遷移時のバッファ確保をゼロにする）
	auto particleManager = ParticleManager::GetInstance();
	particleManager->Initialize();
	particleManager->CreateParticleGroup("anchorHitRay", ModelBuilder::ModelType::kPlane, "resources/textures/gradationLine.png");
	particleManager->CreateParticleGroup("anchorHitFlash", ModelBuilder::ModelType::kPlane, "resources/textures/circle.png");
	particleManager->CreateParticleGroup("enemyDefeatSpark", ModelBuilder::ModelType::kPlane, "resources/textures/circle.png");
	particleManager->CreateParticleGroup("enemyDefeatRing", ModelBuilder::ModelType::kRing, "resources/textures/gradationLine.png");
	particleManager->CreateParticleGroup("enemyChargeRing", ModelBuilder::ModelType::kRing, "resources/textures/gradationLine.png");
	particleManager->CreateParticleGroup("landingDust", ModelBuilder::ModelType::kPlane, "resources/textures/circle.png");

	DirectXCommon::GetInstance()->WaitAndResetCommandList();
	textureManager->ReleaseIntermediateResources();

	// マスターデータ・プレイヤーステータスの読み込み
	PlayerStatusRepository::GetInstance()->Load("resources/data/player_status.json");

	// シーンマネージャーのセットアップ
	auto sceneManager = SceneManager::GetInstance();
	sceneManager->Initialize();
	sceneManager->AddScene("TestScene", std::make_unique<TestScene>());
	sceneManager->AddScene("GameScene", std::make_unique<GameScene>());
	sceneManager->AddScene("TutorialScene", std::make_unique<TutorialScene>());
	sceneManager->AddScene("ResultScene", std::make_unique<ResultScene>());
	sceneManager->AddScene("TitleScene", std::make_unique<TitleScene>());

	sceneManager->ChangeScene("TitleScene");

}

void Core::Run() {
	while (true) {
		if (WinApp::GetInstance()->ProcessMessage()) {
			break;
		}

		NewFrame();

		// 更新処理
		float deltaTime = Time::GetInstance()->GetDeltaTime();
		SceneManager::GetInstance()->Update(deltaTime);


		PreDraw();

		// 描画処理
		SceneManager::GetInstance()->Draw();

		PostDraw();

		SceneManager::GetInstance()->DrawImGui();

		EndFrame();
	}
}

void Core::Finalize() {
	SceneManager::GetInstance()->Finalize();
	Bonjin::Finalize();
}
