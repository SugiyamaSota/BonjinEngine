#include "BattleController.h"

#include "logic/Collision.h"
#include <algorithm>
#include <cmath>
#include <numbers>

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

	particleManager_ = ParticleManager::GetInstance();
	particleManager_->Initialize();
	particleManager_->CreateParticleGroup(
		"anchorHitRay", ModelBuilder::ModelType::kPlane, "resources/textures/gradationLine.png");
	particleManager_->CreateParticleGroup(
		"anchorHitFlash", ModelBuilder::ModelType::kPlane, "resources/textures/circle.png");
	particleManager_->CreateParticleGroup(
		"enemyDefeatSpark", ModelBuilder::ModelType::kPlane, "resources/textures/circle.png");
	particleManager_->CreateParticleGroup(
		"enemyDefeatRing", ModelBuilder::ModelType::kRing, "resources/textures/gradationLine.png");
	particleManager_->Clear("anchorHitRay");
	particleManager_->Clear("anchorHitFlash");
	particleManager_->Clear("enemyDefeatSpark");
	particleManager_->Clear("enemyDefeatRing");
}

void BattleController::Unload() {
	lockedOnEnemies_.clear();
	enemies_.clear();
	enemyModels_.clear();
	player_.reset();
	playerModel_.reset();
	mapChipField_.reset();
	skyBox_.reset();
	particleManager_ = nullptr;
	camera_ = nullptr;
}

void BattleController::Update(float deltaTime) {
	(void)deltaTime;
	skyBox_->Update(InitializeWorldTransform(), camera_);
	player_->Update();
	CheckAnchorEnemyCollision();

	for (const auto& enemy : enemies_) {
		if (enemy->ConsumeDefeatEffectRequest()) {
			EmitEnemyDefeatEffect(enemy->GetWorldPosition());
		}

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

	particleManager_->Update(camera_);
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
	particleManager_->Draw();
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
			const Vector3 hitPosition = anchor.GetPosition();
			enemy->OnCollision();
			lockedOnEnemies_.push_back(enemy.get());
			anchor.OnCollision();
			EmitAnchorHitEffect(hitPosition);
			break;
		}
	}
}

void BattleController::EmitAnchorHitEffect(const Vector3& position) {
	constexpr uint32_t kRayCount = 8;
	std::uniform_real_distribution<float> rotateDistribution(
		-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> lengthDistribution(1.2f, 2.2f);

	for (uint32_t index = 0; index < kRayCount; ++index) {
		ParticleConfig ray{};
		ray.position = position;
		ray.rotate = {0.0f, 0.0f, rotateDistribution(randomEngine_)};
		ray.velocity = {0.0f, 0.0f, 0.0f};
		ray.scale = {0.12f, lengthDistribution(randomEngine_), 1.0f};
		ray.color = {0.35f, 0.75f, 1.0f, 1.0f};
		ray.lifeTime = 0.28f;
		ray.updateFunc = [](ParticleData& particle, float deltaTime) {
			particle.transform.scale.y += 4.0f * deltaTime;
			particle.transform.scale.x -= 0.25f * deltaTime;
			if (particle.transform.scale.x < 0.0f) {
				particle.transform.scale.x = 0.0f;
			}
		};
		particleManager_->Emit("anchorHitRay", ray);
	}

	ParticleConfig flash{};
	flash.position = position;
	flash.scale = {0.9f, 0.9f, 1.0f};
	flash.color = {0.65f, 0.9f, 1.0f, 1.0f};
	flash.lifeTime = 0.18f;
	flash.updateFunc = [](ParticleData& particle, float deltaTime) {
		const float expansion = 4.0f * deltaTime;
		particle.transform.scale.x += expansion;
		particle.transform.scale.y += expansion;
	};
	particleManager_->Emit("anchorHitFlash", flash);
}

void BattleController::EmitEnemyDefeatEffect(const Vector3& position) {
	constexpr uint32_t kSparkCount = 16;
	std::uniform_real_distribution<float> angleDistribution(
		0.0f, std::numbers::pi_v<float> * 2.0f);
	std::uniform_real_distribution<float> speedDistribution(3.0f, 6.0f);
	std::uniform_real_distribution<float> sizeDistribution(0.18f, 0.38f);

	for (uint32_t index = 0; index < kSparkCount; ++index) {
		const float angle = angleDistribution(randomEngine_);
		const float speed = speedDistribution(randomEngine_);

		ParticleConfig spark{};
		spark.position = position;
		spark.velocity = {
			std::cos(angle) * speed,
			std::sin(angle) * speed,
			0.0f
		};
		const float size = sizeDistribution(randomEngine_);
		spark.scale = {size, size, 1.0f};
		spark.color = {1.0f, 0.35f, 0.05f, 1.0f};
		spark.lifeTime = 0.55f;
		spark.updateFunc = [](ParticleData& particle, float deltaTime) {
			particle.transform.translate.x += particle.velocity.x * deltaTime;
			particle.transform.translate.y += particle.velocity.y * deltaTime;
			particle.velocity.y -= 5.0f * deltaTime;

			const float shrink = 0.35f * deltaTime;
			particle.transform.scale.x -= shrink;
			particle.transform.scale.y -= shrink;
			if (particle.transform.scale.x < 0.0f) {
				particle.transform.scale.x = 0.0f;
			}
			if (particle.transform.scale.y < 0.0f) {
				particle.transform.scale.y = 0.0f;
			}
		};
		particleManager_->Emit("enemyDefeatSpark", spark);
	}

	ParticleConfig ring{};
	ring.position = position;
	ring.scale = {0.35f, 0.35f, 1.0f};
	ring.color = {1.0f, 0.65f, 0.1f, 1.0f};
	ring.lifeTime = 0.4f;
	ring.updateFunc = [](ParticleData& particle, float deltaTime) {
		const float expansion = 6.0f * deltaTime;
		particle.transform.scale.x += expansion;
		particle.transform.scale.y += expansion;
	};
	particleManager_->Emit("enemyDefeatRing", ring);
}
