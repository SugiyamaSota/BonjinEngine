#pragma once

#include <list>
#include <memory>
#include <random>
#include <vector>

#include "gameObject/enemy/Enemy.h"
#include "gameObject/player/Player.h"
#include "mapchip/MapChipField.h"
#include "Object3D.h"
#include "ParticleManager.h"
#include "SkyBox.h"

namespace Bonjin {

/// <summary>
/// GameSceneとTutorialSceneがそれぞれ所有するバトル進行処理
/// </summary>
class BattleController {
public:
	void Initialize(Camera* camera, const char* mapFilePath);
	void Unload();
	void Update(float deltaTime);
	void Draw();
	void DrawImGui();

	Player* GetPlayer() const { return player_.get(); }
	MapChipField* GetMapChipField() const { return mapChipField_.get(); }
	void SetEnemyRespawnEnabled(bool enabled) { isEnemyRespawnEnabled_ = enabled; }
	bool IsEnemyRespawnEnabled() const { return isEnemyRespawnEnabled_; }
	bool IsGoalReached() const { return isGoalReached_; }

private:
	Camera* camera_ = nullptr;
	std::unique_ptr<Player> player_;
	std::unique_ptr<Object3D> playerModel_;
	std::list<std::unique_ptr<Enemy>> enemies_;
	std::list<std::unique_ptr<Object3D>> enemyModels_;
	std::list<Enemy*> lockedOnEnemies_;
	std::unique_ptr<MapChipField> mapChipField_;
	std::vector<std::vector<std::unique_ptr<Object3D>>> blockModels_;
	std::vector<std::vector<WorldTransform>> blockWorldTransforms_;
	std::unique_ptr<Object3D> goalModel_;
	WorldTransform goalWorldTransform_{};
	std::unique_ptr<SkyBox> skyBox_;
	ParticleManager* particleManager_ = nullptr;
	std::mt19937 randomEngine_{std::random_device{}()};
	bool isEnemyRespawnEnabled_ = true;
	float enemyRespawnTime_ = 3.0f;
	bool isGoalReached_ = false;

	void GenerateBlocksAndGoal();
	void ResolveAnchorInstantLanding(class Anchor& anchor);
	void CheckAnchorEnemyCollision();
	void EmitAnchorHitEffect(const Vector3& position);
	void EmitEnemyDefeatEffect(const Vector3& position);
	void EmitLandingDustEffect(const Vector3& position);
};

}
