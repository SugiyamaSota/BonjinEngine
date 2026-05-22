#include "GameScene.h"

#include"../system/utility/random/RandomEngine.h"

using namespace Bonjin;

void GameScene::Initialize(Camera* camera)
{

	// 今のシーンと遷移後シーン(初期値は同じ)
	currentSceneType_ = SceneType::kGame;
	nextSceneType_ = SceneType::kGame;

	this->camera_ = camera;

	mapChipField_ = std::make_unique<MapChipField>();
	mapChipField_->LoadmapChipCsv("resources/maps/tutorial.csv");

	blockModel_ = std::make_unique<Object3D>();
	blockModel_->CreateModel(ModelBuilder::ModelType::kCube, "resources/textures/cube.jpg");
	blockModel_->SetEnableEnableEnvironmentMap(false);


	GenerateBlocksAndGoal();

	playerModel_ = std::make_unique<Object3D>();
	playerModel_->CreateModel(ModelBuilder::ModelType::kSphere, "resources/textures/default.png");
	playerModel_->SetEnableEnableEnvironmentMap(false);
	player_ = std::make_unique<Player>();
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(2, 1);
	player_->Initialize(playerModel_.get(), camera_, playerPosition);
	player_->SetMapChipField(mapChipField_.get());

}

void GameScene::Unload() {
}

void GameScene::Update(float deltaTime) {

	player_->Update();

	for (const auto& enemy : enemies_) {
		enemy->Update();
	}

	// プレイヤーを追従
	camera_->SetTarget(player_->GetPosition());

	for (const auto& transform : blockWorldTransforms_) {
		blockModel_->Update(transform, camera_);
	}

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

	for (const auto& transform : blockWorldTransforms_) {
		blockModel_->Draw();
	}

	player_->Draw();

	for (const auto& enemy : enemies_) {
		enemy->Draw();
	}

	
}

void GameScene::DrawSceneImGui() {
#ifdef USE_IMGUI

	LightManager::GetInstance()->DrawImGui();

#endif
}

SceneType GameScene::GetNextScene() const {
	return nextSceneType_;
}

void GameScene::GenerateBlocksAndGoal() {
	uint32_t numBlockVirtical = mapChipField_->GetNumBlockVertical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	for (uint32_t i = 0; i < numBlockVirtical; ++i) {
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				// 【変更】ブロックが必要な座標のトランスフォームを新しく作って追加
				WorldTransform transform;
				transform.rotate = { 0, 0, 0 };
				transform.scale = { 1, 1, 1 };
				transform.translate = mapChipField_->GetMapChipPositionByIndex(j, i);

				blockWorldTransforms_.push_back(transform);
			} else if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kGoal) {
				goalWorldTransform_.rotate = { 0,0,0 };
				goalWorldTransform_.scale = { 1,1,1 };
				goalWorldTransform_.translate = mapChipField_->GetMapChipPositionByIndex(j, i);
			}
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kEnemy) {
				// 新しいモデルを生成
				enemyModels_.push_back(std::make_unique<Object3D>());

				enemyModels_.back()->CreateModel(ModelBuilder::ModelType::kSphere, "resources/textures/default.png");
				enemyModels_.back()->SetEnableEnableEnvironmentMap(false);
				enemyModels_.back()->SetColor({ 0.2f,0.2f,0.2f,1.f });

				// 新しい敵を生成
				enemies_.push_back(std::make_unique<Enemy>());
				// 敵の位置を縦に並べる
				Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(j, i);
				// 生成したモデルを敵に渡して初期化
				enemies_.back()->Initialize(enemyModels_.back().get(), camera_, enemyPosition);
			}
		}
	}
}