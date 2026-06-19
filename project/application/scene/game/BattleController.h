#pragma once

#include <list>
#include <memory>
#include <random>

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

private:
	static const uint32_t kNumBlockVirtical = 11;
	static const uint32_t kNumBlockHorizontal = 21;

	Camera* camera_ = nullptr;
	std::unique_ptr<Player> player_;
	std::unique_ptr<Object3D> playerModel_;
	std::list<std::unique_ptr<Enemy>> enemies_;
	std::list<std::unique_ptr<Object3D>> enemyModels_;
	std::list<Enemy*> lockedOnEnemies_;
	std::unique_ptr<MapChipField> mapChipField_;
	std::unique_ptr<Object3D> blockModel_[kNumBlockVirtical][kNumBlockHorizontal] = {};
	WorldTransform blockWorldTransform_[kNumBlockVirtical][kNumBlockHorizontal] = {};
	WorldTransform goalWorldTransform_{};
	std::unique_ptr<SkyBox> skyBox_;
	ParticleManager* particleManager_ = nullptr;
	std::mt19937 randomEngine_{std::random_device{}()};

	void GenerateBlocksAndGoal();
	void CheckAnchorEnemyCollision();
	void EmitAnchorHitEffect(const Vector3& position);
	void EmitEnemyDefeatEffect(const Vector3& position);
};

}
