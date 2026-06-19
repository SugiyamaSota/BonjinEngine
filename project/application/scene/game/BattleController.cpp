#include "BattleController.h"

#include "logic/Collision.h"

using namespace Bonjin;

void BattleController::Initialize(Camera* camera, const char* mapFilePath) {
	camera_ = camera;
	enemies_.clear();
	enemyModels_.clear();
	lockedOnEnemies_.clear();

	mapChipField_ = std::make_unique<MapChipField>();
	mapChipField_->LoadmapChipCsv(mapFilePath);

	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			blockModel_[i][j] = std::make_unique<Object3D>();
			blockModel_[i][j]->CreateModel(
				ModelBuilder::ModelType::kCube, "resources/textures/cube.jpg");
			blockModel_[i][j]->SetEnableEnableEnvironmentMap(false);
			blockWorldTransform_[i][j] = InitializeWorldTransform();
		}
	}

	GenerateBlocksAndGoal();

	playerModel_ = std::make_unique<Object3D>();
	playerModel_->CreateModel(
		ModelBuilder::ModelType::kSphere, "resources/textures/default.png");
	playerModel_->SetEnableEnableEnvironmentMap(false);

	player_ = std::make_unique<Player>();
	const Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(2, 1);
	player_->Initialize(playerModel_.get(), camera_, playerPosition);
	player_->SetMapChipField(mapChipField_.get());
	player_->SetLockedOnEnemiesList(&lockedOnEnemies_);

	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) != MapChipType::kEnemy) {
				continue;
			}

			enemyModels_.push_back(std::make_unique<Object3D>());
			enemyModels_.back()->CreateModel(
				ModelBuilder::ModelType::kSphere, "resources/textures/default.png");
			enemyModels_.back()->SetEnableEnableEnvironmentMap(false);
			enemyModels_.back()->SetColor({0.2f, 0.2f, 0.2f, 1.0f});

			enemies_.push_back(std::make_unique<Enemy>());
			const Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(j, i);
			enemies_.back()->Initialize(enemyModels_.back().get(), camera_, enemyPosition);
			enemies_.back()->SetMapChipField(mapChipField_.get());
		}
	}

	skyBox_ = std::make_unique<SkyBox>();
	skyBox_->CreateModel(
		ModelBuilder::ModelType::kSkyBox, "resources/textures/skyBox.dds");
}

void BattleController::Unload() {
	lockedOnEnemies_.clear();
	enemies_.clear();
	enemyModels_.clear();
	player_.reset();
	playerModel_.reset();
	mapChipField_.reset();
	skyBox_.reset();
	camera_ = nullptr;
}

void BattleController::Update(float deltaTime) {
	(void)deltaTime;
	skyBox_->Update(InitializeWorldTransform(), camera_);
	player_->Update();
	CheckAnchorEnemyCollision();

	for (const auto& enemy : enemies_) {
		if (!enemy->GetIsDead()) {
			enemy->Update();
		}
	}

	const IndexSet centerIndex = mapChipField_->GetMapChipIndexSetByCenter();
	camera_->SetTarget(
		mapChipField_->GetMapChipPositionByIndex(centerIndex.xIndex, centerIndex.yIndex));

	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				blockModel_[i][j]->Update(blockWorldTransform_[i][j], camera_);
			}
		}
	}
}

void BattleController::Draw() {
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
	skyBox_->Draw();
}

void BattleController::DrawImGui() {
#ifdef USE_IMGUI
	LightManager::GetInstance()->DrawImGui();
#endif
}

void BattleController::GenerateBlocksAndGoal() {
	const uint32_t vertical = mapChipField_->GetNumBlockVirtical();
	const uint32_t horizontal = mapChipField_->GetNumBlockHorizontal();

	for (uint32_t i = 0; i < vertical; ++i) {
		for (uint32_t j = 0; j < horizontal; ++j) {
			const MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);
			if (type == MapChipType::kBlock) {
				blockWorldTransform_[i][j] = InitializeWorldTransform();
				blockWorldTransform_[i][j].translate =
					mapChipField_->GetMapChipPositionByIndex(j, i);
			} else if (type == MapChipType::kGoal) {
				goalWorldTransform_ = InitializeWorldTransform();
				goalWorldTransform_.translate =
					mapChipField_->GetMapChipPositionByIndex(j, i);
			}
		}
	}
}

void BattleController::CheckAnchorEnemyCollision() {
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
