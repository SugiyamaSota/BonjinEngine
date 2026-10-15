#include "Core.h"
#include "BonjinEngine.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "GameScene.h"
#include "../scene/tutorial/TutorialScene.h"
#include "../scene/result/ResultScene.h"
#include "gameObject/player/PlayerStatusRepository.h"
#include "TestScene.h"

#include "ParticleManager.h"

using namespace Bonjin;

Core::Core() {}
Core::~Core() {}

void Core::Initialize() {
	Bonjin::Initialize(); // エンジン本体の初期化

	///--- シーンのセットアップ ---
	auto sceneManager = SceneManager::GetInstance();
	sceneManager->Initialize();
	// 各シーンの追加
	sceneManager->AddScene("TitleScene", std::make_unique<TitleScene>());
	sceneManager->AddScene("TutorialScene", std::make_unique<TutorialScene>());
	sceneManager->AddScene("GameScene", std::make_unique<GameScene>());
	sceneManager->AddScene("ResultScene", std::make_unique<ResultScene>());
	sceneManager->AddScene("TestScene", std::make_unique<TestScene>());
	// 最初のシーンを設定
	sceneManager->ChangeScene("TitleScene");

	///--- 起動時にリソースのロード ---
	// モデル
	auto modelManager = ModelManager::GetInstance();
	modelManager->LoadModel("human", "resources/models/human", "walk.gltf");
	// テクスチャ
	auto textureManager = TextureManager::GetInstance();
	textureManager->LoadTexture("resources/textures/cube.jpg");
	textureManager->LoadTexture("resources/textures/default.png");
	textureManager->LoadTexture("resources/textures/SkyBox_Dark.dds");
	textureManager->LoadTexture("resources/textures/skyBox.dds");
	textureManager->LoadTexture("resources/textures/gradationLine.png");
	textureManager->LoadTexture("resources/textures/circle.png");
	textureManager->LoadTexture("resources/textures/noise0.png");
	// パーティクルグループ
	auto particleManager = ParticleManager::GetInstance();
	particleManager->Initialize();
	particleManager->CreateParticleGroup("anchorHitRay", ModelBuilder::ModelType::kPlane, "resources/textures/gradationLine.png");
	particleManager->CreateParticleGroup("anchorHitFlash", ModelBuilder::ModelType::kPlane, "resources/textures/circle.png");
	particleManager->CreateParticleGroup("enemyDefeatSpark", ModelBuilder::ModelType::kPlane, "resources/textures/circle.png");
	particleManager->CreateParticleGroup("enemyDefeatRing", ModelBuilder::ModelType::kRing, "resources/textures/gradationLine.png");
	particleManager->CreateParticleGroup("enemyChargeRing", ModelBuilder::ModelType::kRing, "resources/textures/gradationLine.png");
	particleManager->CreateParticleGroup("landingDust", ModelBuilder::ModelType::kPlane, "resources/textures/circle.png");

	DirectXCommon::GetInstance()->WaitAndResetCommandList();

	///--- ゲーム固有データの読み込み
	// プレイヤーのレベルごとのステータステーブル
	PlayerStatusRepository::GetInstance()->Load("resources/data/player_status.json");

}

void Core::Run() {
	while (true) {
		if (WinApp::GetInstance()->ProcessMessage()) {
			break;
		}

		NewFrame();

		// 更新処理
		SceneManager::GetInstance()->Update(Time::GetInstance()->GetDeltaTime());

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
