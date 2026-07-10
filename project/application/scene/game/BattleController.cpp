#include "BattleController.h"

#include "logic/Collision.h"
#include "logic/CollisionManager.h"
#include "gameObject/anchor/anchor.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Bonjin;

void BattleController::Initialize(Camera* camera, const char* mapFilePath) {
	CollisionManager::GetInstance()->Clear();

	camera_ = camera;
	enemies_.clear();
	enemyModels_.clear();
	lockedOnEnemies_.clear();

	mapChipField_ = std::make_unique<MapChipField>();
	mapChipField_->LoadmapChipCsv(mapFilePath);

	const uint32_t vertical = mapChipField_->GetNumBlockVirtical();
	const uint32_t horizontal = mapChipField_->GetNumBlockHorizontal();

	blockModels_.resize(vertical);
	blockWorldTransforms_.resize(vertical);
	for (uint32_t i = 0; i < vertical; ++i) {
		blockModels_[i].resize(horizontal);
		blockWorldTransforms_[i].resize(horizontal);
		for (uint32_t j = 0; j < horizontal; ++j) {
			blockModels_[i][j] = std::make_unique<Object3D>();
			blockModels_[i][j]->CreateModel(
				ModelBuilder::ModelType::kCube, "resources/textures/cube.jpg");
			blockModels_[i][j]->SetEnableEnableEnvironmentMap(false);
			blockWorldTransforms_[i][j] = InitializeWorldTransform();
		}
	}

	isGoalReached_ = false;

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

	for (uint32_t i = 0; i < vertical; ++i) {
		for (uint32_t j = 0; j < horizontal; ++j) {
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
			enemies_.back()->SetPlayer(player_.get());
		}
	}

	skyBox_ = std::make_unique<SkyBox>();
	skyBox_->CreateModel(
		ModelBuilder::ModelType::kSkyBox, "resources/textures/SkyBox_Dark.dds");

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
	particleManager_->CreateParticleGroup(
		"enemyChargeRing", ModelBuilder::ModelType::kRing, "resources/textures/gradationLine.png");
	particleManager_->CreateParticleGroup(
		"landingDust", ModelBuilder::ModelType::kPlane, "resources/textures/circle.png");
	particleManager_->Clear("anchorHitRay");
	particleManager_->Clear("anchorHitFlash");
	particleManager_->Clear("enemyDefeatSpark");
	particleManager_->Clear("enemyDefeatRing");
	particleManager_->Clear("landingDust");
}

void BattleController::Unload() {
	lockedOnEnemies_.clear();
	enemies_.clear();
	enemyModels_.clear();
	player_.reset();
	playerModel_.reset();
	goalModel_.reset();
	goal_.reset();
	blockModels_.clear();
	blockWorldTransforms_.clear();
	mapChipField_.reset();
	skyBox_.reset();
	particleManager_ = nullptr;
	camera_ = nullptr;
}

void BattleController::Update(float deltaTime) {
	(void)deltaTime;
	skyBox_->Update(InitializeWorldTransform(), camera_);
	player_->Update();

	// アンカーの即着地処理
	if (player_->HasAnchor()) {
		Anchor& anchor = player_->GetAnchor();
		if (!anchor.GetStandBy() && !anchor.IsInstantResolved()) {
			ResolveAnchorInstantLanding(anchor);
		}
	}

	if (goal_) {
		goal_->Update();
	}

	if (player_->ConsumeLandingEffectRequest()) {
		Vector3 dustPosition = player_->GetPosition();
		dustPosition.y -= 1.0f;
		dustPosition.z = -0.1f;
		EmitLandingDustEffect(dustPosition);
	}

	// 共通衝突判定の実行
	CollisionManager::GetInstance()->CheckAllCollisions();

	// プレイヤーのゴール到達フラグチェック
	if (player_ && player_->GetIsGoalReached()) {
		isGoalReached_ = true;
	}

	for (const auto& enemy : enemies_) {
		if (enemy->ConsumeDefeatEffectRequest()) {
			EmitEnemyDefeatEffect(enemy->GetWorldPosition());
		}

		if (enemy->GetIsDead()) {
			enemy->UpdateRespawn(deltaTime, enemyRespawnTime_, isEnemyRespawnEnabled_);
		} else {
			enemy->Update();
		}
	}

	//const IndexSet centerIndex = mapChipField_->GetMapChipIndexSetByCenter();
	camera_->SetTarget(
		player_->GetWorldPosition());

	const uint32_t vertical = mapChipField_->GetNumBlockVirtical();
	const uint32_t horizontal = mapChipField_->GetNumBlockHorizontal();
	for (uint32_t i = 0; i < vertical; ++i) {
		for (uint32_t j = 0; j < horizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				blockModels_[i][j]->Update(blockWorldTransforms_[i][j], camera_);
			}
		}
	}

	particleManager_->Update(camera_);
}

void BattleController::Draw() {
	const uint32_t vertical = mapChipField_->GetNumBlockVirtical();
	const uint32_t horizontal = mapChipField_->GetNumBlockHorizontal();
	for (uint32_t i = 0; i < vertical; ++i) {
		for (uint32_t j = 0; j < horizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				blockModels_[i][j]->Draw();
			}
		}
	}

	if (goal_) {
		goal_->Draw();
	}

	player_->Draw();
	for (const auto& enemy : enemies_) {
		if (!enemy->GetIsDead()) {
			enemy->Draw();
		}
	}
	skyBox_->Draw();
	particleManager_->Draw();

	if (player_) {
		player_->DrawAnchorLine();
	}
}

void BattleController::DrawImGui() {
#ifdef USE_IMGUI
	LightManager::GetInstance()->DrawImGui();
	ImGui::Checkbox("Enemy Respawn", &isEnemyRespawnEnabled_);
	ImGui::SliderFloat("Enemy Respawn Time", &enemyRespawnTime_, 0.5f, 10.0f, "%.1f sec");

	if (player_) {
		player_->DrawImGui();
	}

	if (ImGui::TreeNode("Enemies")) {
		int index = 0;
		for (const auto& enemy : enemies_) {
			enemy->DrawImGui(index++);
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Goal Debug")) {
		ImGui::Text("Goal Reached: %s", isGoalReached_ ? "TRUE" : "FALSE");
		if (goal_) {
			Vector3 goalPos = goal_->GetPosition();
			ImGui::Text("Goal Pos: (%.2f, %.2f, %.2f)", goalPos.x, goalPos.y, goalPos.z);
			AABB pAABB = player_->GetAABB();
			ImGui::Text("Player AABB min: (%.2f, %.2f, %.2f)", pAABB.min.x, pAABB.min.y, pAABB.min.z);
			ImGui::Text("Player AABB max: (%.2f, %.2f, %.2f)", pAABB.max.x, pAABB.max.y, pAABB.max.z);

			AABB gAABB = goal_->GetAABB();
			ImGui::Text("Goal AABB min: (%.2f, %.2f, %.2f)", gAABB.min.x, gAABB.min.y, gAABB.min.z);
			ImGui::Text("Goal AABB max: (%.2f, %.2f, %.2f)", gAABB.max.x, gAABB.max.y, gAABB.max.z);

			bool collision = IsCollision(pAABB, gAABB);
			ImGui::Text("Is Collision: %s", collision ? "TRUE" : "FALSE");
		} else {
			ImGui::Text("Goal is NULL (No Goal in Map)");
		}
		ImGui::TreePop();
	}
#endif
}

void BattleController::GenerateBlocksAndGoal() {
	const uint32_t vertical = mapChipField_->GetNumBlockVirtical();
	const uint32_t horizontal = mapChipField_->GetNumBlockHorizontal();

	for (uint32_t i = 0; i < vertical; ++i) {
		for (uint32_t j = 0; j < horizontal; ++j) {
			const MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);
			if (type == MapChipType::kBlock) {
				blockWorldTransforms_[i][j] = InitializeWorldTransform();
				blockWorldTransforms_[i][j].translate =
					mapChipField_->GetMapChipPositionByIndex(j, i);
			} else if (type == MapChipType::kGoal) {
				goalModel_ = std::make_unique<Object3D>();
				goalModel_->CreateModel(
					ModelBuilder::ModelType::kCube, "resources/textures/cube.jpg");
				goalModel_->SetEnableEnableEnvironmentMap(false);
				goalModel_->SetColor({0.0f, 1.0f, 0.0f, 1.0f}); // ゴールを緑色にする

				goal_ = std::make_unique<GameObject>();
				GameObjectInitConfig config;
				config.physicsType = PhysicsType::None;
				config.hasCollider = true;
				config.categoryAttr = kAttributeGoal;
				config.collisionMask = kAttributePlayer;
				config.tag = "Goal";

				const Vector3 goalPosition = mapChipField_->GetMapChipPositionByIndex(j, i);
				goal_->Initialize(goalModel_.get(), camera_, goalPosition, config);
				goal_->SetWidth(2.2f);
				goal_->SetHeight(2.2f);
			}
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

void BattleController::EmitLandingDustEffect(const Vector3& position) {
	constexpr uint32_t kDustCount = 10;
	std::uniform_real_distribution<float> horizontalSpeed(-3.2f, 3.2f);
	std::uniform_real_distribution<float> upwardSpeed(0.8f, 2.0f);
	std::uniform_real_distribution<float> sizeDistribution(0.25f, 0.55f);
	std::uniform_real_distribution<float> positionOffset(-0.5f, 0.5f);

	for (uint32_t index = 0; index < kDustCount; ++index) {
		ParticleConfig dust{};
		dust.position = {
			position.x + positionOffset(randomEngine_),
			position.y,
			position.z
		};
		dust.velocity = {
			horizontalSpeed(randomEngine_),
			upwardSpeed(randomEngine_),
			0.0f
		};
		const float size = sizeDistribution(randomEngine_);
		dust.scale = {size * 1.4f, size, 1.0f};
		dust.color = {0.55f, 0.42f, 0.25f, 0.7f};
		dust.lifeTime = 0.55f;
		dust.updateFunc = [](ParticleData& particle, float deltaTime) {
			particle.transform.translate.x += particle.velocity.x * deltaTime;
			particle.transform.translate.y += particle.velocity.y * deltaTime;
			particle.velocity.x *= 0.94f;
			particle.velocity.y -= 2.5f * deltaTime;

			particle.transform.scale.x += 0.7f * deltaTime;
			particle.transform.scale.y += 0.35f * deltaTime;
		};
		particleManager_->Emit("landingDust", dust);
	}
}

void BattleController::ResolveAnchorInstantLanding(Anchor& anchor) {
	Vector3 start = anchor.GetPosition();
	Vector3 velocity = anchor.GetVelocity();
	if (Length(velocity) <= 0.001f) {
		return;
	}
	Vector3 dir = Normalize(velocity);

	const float stepLen = 0.1f;
	const float maxDist = 50.0f;
	float currentDist = 0.0f;

	Vector3 currentPos = start;
	bool hit = false;

	while (currentDist < maxDist) {
		currentPos = Add(start, Multiply(currentDist, dir));

		IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(currentPos);
		MapChipType type = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		if (type == MapChipType::kBlock) {
			// めり込み防止のためにアンカーの半径分（0.5f）＋マージン（0.05f）を逆方向に引き戻す
			const float pullBackDist = 0.55f;
			Vector3 hitPos = Subtract(currentPos, Multiply(pullBackDist, dir));
			anchor.SetPosition(hitPos);
			anchor.SetStandBy(true);
			hit = true;
			break;
		}

		AABB tempAABB;
		float w = 1.0f;
		float h = 1.0f;
		tempAABB.min = { currentPos.x - w / 2.0f, currentPos.y - h / 2.0f, -0.5f };
		tempAABB.max = { currentPos.x + w / 2.0f, currentPos.y + h / 2.0f, 0.5f };

		Enemy* hitEnemy = nullptr;
		for (const auto& enemy : enemies_) {
			if (enemy->GetIsDead() || enemy->GetIsLockedOn()) {
				continue;
			}
			if (IsCollision(tempAABB, enemy->GetAABB())) {
				hitEnemy = enemy.get();
				break;
			}
		}

		if (hitEnemy) {
			anchor.SetPosition(currentPos);
			anchor.OnCollision();
			hitEnemy->OnCollision();
			lockedOnEnemies_.push_back(hitEnemy);
			EmitAnchorHitEffect(currentPos);
			hit = true;
			break;
		}

		currentDist += stepLen;
	}

	if (!hit) {
		anchor.SetPosition(currentPos);
		anchor.SetDead(true);
	}

	anchor.SetInstantResolved(true);
}
