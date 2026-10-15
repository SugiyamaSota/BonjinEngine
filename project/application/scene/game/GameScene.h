#pragma once

#include "../interface/BaseScene.h"
#include "BattleController.h"

namespace Bonjin {

enum class GamePhase {
	kHighlight,
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
		std::unique_ptr<BattleController> battleController_;

		// --- ハイライト演出用 ---
		enum class HighlightState {
			kDissolveIn,
			kCameraPan,
			kDissolveOut,
			kPlayerFocusIn
		};

		HighlightState highlightState_ = HighlightState::kDissolveIn;
		size_t currentHighlightIndex_ = 0;
		float highlightTimer_ = 0.0f;
		static constexpr float kDissolveDuration = 0.5f;

		void UpdateHighlightPhase(float deltaTime);
		void FinishHighlight();
		void UpdatePlayPhase(float deltaTime);
	};

}
