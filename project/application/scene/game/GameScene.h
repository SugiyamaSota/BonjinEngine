#pragma once
#include "../interface/BaseScene.h" // ISceneをインクルード

#include"BonjinEngine.h"

#include"player/Player.h"
#include"enemy/Enemy.h"
#include"enemy/Debris.h"
#include"mapchip/MapChipField.h"
#include"others/Collision.h"
#include <list>
#include <memory>

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
		Vector3 cameraTarget_; // カメラのターゲット座標
		static inline const float kCameraLerpRate = 0.05f; // カメラの追従速度（補間率）

		//--- プレイヤー関連 ---
		std::unique_ptr<Player> player_ = nullptr;
		WorldTransform playerWT_;
		std::unique_ptr<Model> playerModel_ = nullptr;
		void PlayerInit();

		//--- 敵関連 ---
		std::list<std::unique_ptr<Enemy>> enemies_;
		std::list<std::unique_ptr<Model>> enemyModels_;
		std::list<Enemy*> lockedOnEnemies_;
		static const uint32_t kMaxLockedOnEnemies = 3;
		std::list<std::unique_ptr<Debris>> debris_;
		void EnemyInit();

		// --- ブロックとゴール関連 ---
		static const uint32_t kNumBlockVirtical = 10;
		static const uint32_t kNumBlockHorizontal = 60;
		std::unique_ptr<Model> blockModel_[kNumBlockVirtical][kNumBlockHorizontal];
		WorldTransform blockWorldTransform_[kNumBlockVirtical][kNumBlockHorizontal];
		std::unique_ptr<MapChipField> mapChipField_;
		void GenerateBlocksAndGoal();
		std::unique_ptr<Model> goalModel_ = nullptr;
		WorldTransform goalWorldTransform_;
		bool isGoal_;
		Vector3 goalPosition = { 0,0,0 };
		void BlocksAndGoalInit();

		// スプライト
		std::unique_ptr<Sprite> HUD_Tab_;
		std::unique_ptr<Sprite> HUD_Underbar_ = nullptr;
		std::unique_ptr<Sprite> HUD_Default_ = nullptr;
		std::unique_ptr<Sprite> HUD_Anchor_ = nullptr;
		std::unique_ptr<Sprite> HUD_Destroy_ = nullptr;
		std::vector<std::unique_ptr<Sprite>> tutrialSprites_;
		int currentTutrialPage_ = 0;
		bool showTutrial;
		std::unique_ptr<Sprite> gameClearSprite_ = nullptr;
		void HUDInit();
		void DrawHUD();

		// 開始フェーズの演出時間
		static inline const float kStartTime = 5.0f;

		// シーン変更
		bool sceneChangeStandby_;


		bool canGoal_ = false;

		bool showFirstTutrial_ = false;

		/// <summary>
		/// 当たり判定
		/// </summary>
		void CheckAllCollisions();

		/// <summary>
		/// ゴールとの判定
		/// </summary>
		void CheckGoal();

		float fadeTimer_;

	private:
		// --- ゲーム固有の関数 ---
	};
}