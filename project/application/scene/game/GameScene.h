#pragma once
#include "../interface/BaseScene.h" // ISceneをインクルード

#include<memory>

#include "gameObject/player/Player.h"
#include "gameObject/enemy/Enemy.h"
#include "mapchip/MapChipField.h"

#include "Object3D.h"

namespace Bonjin
{

	enum class GamePhase {
		kStart,
		kPlay,
		kGoal,
	};

	class GameScene : public BaseScene<GamePhase>
	{
	public:
		// --- オーバーライド関数 --- 
		virtual ~GameScene() = default;

		void Initialize(Camera* camera) override;

		void Unload()override;

		void Update(float deltaTime) override;

		void Draw() override;

		SceneType GetNextScene() const override;

		void DrawSceneImGui() override;

		const char* GetScenename()const override {
			return "GameScene";
		}

	private:

		// --- ゲーム固有の変数 ---
		std::unique_ptr<Player> player_ = nullptr;
		std::unique_ptr<Object3D> playerModel_ = nullptr;

		std::list<std::unique_ptr<Enemy>> enemies_;
		std::list<std::unique_ptr<Object3D>> enemyModels_;


		std::unique_ptr<MapChipField> mapChipField_ = nullptr;
		std::unique_ptr<Object3D> blockModel_ = nullptr;
		std::vector<WorldTransform> blockWorldTransforms_;
		std::unique_ptr<Object3D> goalModel_ = nullptr;
		WorldTransform goalWorldTransform_;

	private:
		// --- ゲーム固有の関数 ---
		/// <summary>
		/// ブロックとゴール生成
		/// </summary>
		void GenerateBlocksAndGoal();

	};
}