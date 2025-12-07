#pragma once
#include "../base/SceneBase.h"

#include"../../player/Player.h"
#include"../../enemy/Enemy.h"
#include"../../enemy/Debris.h"
#include"../../field/Skydome.h"
#include"../../mapchip/MapChipField.h"
#include <list>
#include <memory>

// シーンの状態を表す列挙型
enum class GamePhase {
    kStart, // 開始前（カメラ演出）
    kPlay,  // ゲームプレイ中
    kGoal,  // クリア後
};

namespace BonjinEngine {

    class GameScene : public SceneBase {
    public:
        // 💡 仮想デストラクタは必須
        virtual ~GameScene();

        /// <summary>
        /// シーン初期化
        /// </summary>
        void Initialize(Camera* camera) override;

        /// <summary>
        /// 毎フレーム更新
        /// </summary>
        /// <param name="deltaTime">デルタタイム</param>
        void Update(float deltaTime) override;

        /// <summary>
        /// 毎フレーム描画
        /// </summary>
        void Draw() override;

        /// <summary>
        /// 次のシーンを取得
        /// </summary>
        /// <returns>次のシーンタイプ</returns>
        SceneType GetNextScene() const override;

    private:
		Vector3 cameraTarget_; // カメラのターゲット座標
		static inline const float kCameraLerpRate = 0.05f; // カメラの追従速度（補間率）

		//--- プレイヤー関連 ---
		std::unique_ptr<Player> player_ = nullptr;
		WorldTransform worldTransform_;
		// モデルをunique_ptrに変更
		std::unique_ptr<Model> model_ = nullptr;

		//--- 敵関連 ---
		std::list<std::unique_ptr<Enemy>> enemies_;
		std::list<std::unique_ptr<Model>> enemyModels_;
		std::list<Enemy*> lockedOnEnemies_;
		static const uint32_t kMaxLockedOnEnemies = 3;

		std::list<std::unique_ptr<Debris>> debris_;

		// --- 天球関連 ---
		std::unique_ptr<Skydome> skydome_ = nullptr;
		// モデルをunique_ptrに変更
		std::unique_ptr<Model> skydomeModel_ = nullptr;

		// --- ブロック関連 ---
		static const uint32_t kNumBlockVirtical = 10;
		static const uint32_t kNumBlockHorizontal = 60;
		Model* blockModel_[kNumBlockVirtical][kNumBlockHorizontal] = { nullptr };
		WorldTransform blockWorldTransform_[kNumBlockVirtical][kNumBlockHorizontal];
		std::unique_ptr<MapChipField> mapChipField_;

		// --- ゴール関連 ---
		Model* goalModel_ = nullptr;
		WorldTransform goalWorldTransform_;
		bool isGoal_;
		Vector3 goalPosition = { 0,0,0 };

		// UIとか
		Sprite* HUD;
		std::vector<std::unique_ptr<Sprite>> tutrialSprites_;
		int currentTutrialPage_ = 0;
		bool showTutrial;

		Sprite* gameClearSprite_ = nullptr;
		Sprite* blackScreenSprite_ = nullptr;
		float fadeTimer_ = 0.0f;

		// 現在のフェーズ
		GamePhase phase_ = GamePhase::kStart;
		float fadeInAlpha;
		// フェーズ開始からの経過時間
		float phaseTimer_ = 0.0f;
		// 開始フェーズの演出時間
		static inline const float kStartTime = 5.0f;

		// シーン変更
		bool sceneChangeStandby_;

		/// <summary>
		/// ブロック生成
		/// </summary>
		void GenerateBlocksAndGoal();

		/// <summary>
		/// 当たり判定
		/// </summary>
		void CheckAllCollisions();

		/// <summary>
		/// ゴールとの判定
		/// </summary>
		void CheckGoal();

	public:
		bool GetIsGoal() { return isGoal_; }

		bool GetSceneChengeStandby() { return sceneChangeStandby_; }
    };
}