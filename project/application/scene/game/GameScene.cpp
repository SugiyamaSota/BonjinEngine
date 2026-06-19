#include "GameScene.h"

#include"../system/utility/random/RandomEngine.h"
#include "logic/Collision.h"

using namespace Bonjin;

void GameScene::Initialize(Camera* camera)
{

	// 今のシーンと遷移後シーン(初期値は同じ)
	currentSceneType_ = SceneType::kGame;
	nextSceneType_ = SceneType::kGame;

	this->camera_ = camera;

	mapChipField_ = std::make_unique<MapChipField>();
	mapChipField_->LoadmapChipCsv("resources/maps/tutorial.csv");

	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			blockModel_[i][j] = std::make_unique<Object3D>();
			blockModel_[i][j]->CreateModel(ModelBuilder::ModelType::kCube, "resources/textures/cube.jpg");
			blockModel_[i][j]->SetEnableEnableEnvironmentMap(false);
			blockWorldTransform_[i][j].rotate = { 0,0,0 };
			blockWorldTransform_[i][j].scale = { 1,1,1 };
			blockWorldTransform_[i][j].translate = { 0,0,0 };
		}
	}


	GenerateBlocksAndGoal();

	playerModel_ = std::make_unique<Object3D>();
	playerModel_->CreateModel(ModelBuilder::ModelType::kSphere, "resources/textures/default.png");
	playerModel_->SetEnableEnableEnvironmentMap(false);
	player_ = std::make_unique<Player>();
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(2, 1);
	player_->Initialize(playerModel_.get(), camera_, playerPosition);
	player_->SetMapChipField(mapChipField_.get());
	player_->SetLockedOnEnemiesList(&lockedOnEnemies_);

	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
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

	testSkyBox_ = std::make_unique<SkyBox>();
	testSkyBox_->CreateModel(ModelBuilder::ModelType::kSkyBox, "resources/textures/skyBox.dds");

}

void GameScene::Unload() {
}

void GameScene::Update(float deltaTime) {

	testSkyBox_->Update(InitializeWorldTransform(), camera_);

	player_->Update();
	CheckAnchorEnemyCollision();

	for (const auto& enemy : enemies_) {
		if (!enemy->GetIsDead()) {
			enemy->Update();
		}
	}

	// プレイヤーを追従
	IndexSet currentIS = mapChipField_->GetMapChipIndexSetByCenter();
	Vector3 currentPos = mapChipField_->GetMapChipPositionByIndex(currentIS.xIndex, currentIS.yIndex);
	camera_->SetTarget(currentPos);

	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				blockModel_[i][j]->Update(blockWorldTransform_[i][j], camera_);
			}
		}
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

	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				blockModel_[i][j]->Draw();
			}
		}
	}

	player_->Draw();

	for (const auto& enemy : enemies_) {
		if (!enemy->GetIsDead()) {
			enemy->Draw();
		}
	}

	testSkyBox_->Draw();
}

void GameScene::CheckAnchorEnemyCollision() {
	if (!player_->HasAnchor()) {
		return;
	}

	Anchor& anchor = player_->GetAnchor();
	for (const auto& enemy : enemies_) {
		if (enemy->GetIsDead() || enemy->GetIsLockedOn()) {
			continue;
		}

		if (IsCollision(anchor.GetAABB(), enemy->GetAABB())) {
			enemy->OnCollision();
			lockedOnEnemies_.push_back(enemy.get());
			anchor.OnCollision();
			break;
		}
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
	uint32_t numBlockVirtical = mapChipField_->GetNumBlockVirtical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	const float kBlockWidth = 2.0f;
	const float kBlockHeight = 2.0f;

	for (uint32_t i = 0; i < numBlockVirtical; ++i) {
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				blockWorldTransform_[i][j].rotate = { 0,0,0 };
				blockWorldTransform_[i][j].scale = { 1,1,1 };
				blockWorldTransform_[i][j].translate = mapChipField_->GetMapChipPositionByIndex(j, i);
			} else
				if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kGoal) {
					goalWorldTransform_.rotate = { 0,0,0 };
					goalWorldTransform_.scale = { 1,1,1 };
					goalWorldTransform_.translate = mapChipField_->GetMapChipPositionByIndex(j, i);
					//goalPosition = goalWorldTransform_.translate;
				}
		}
	}
}
