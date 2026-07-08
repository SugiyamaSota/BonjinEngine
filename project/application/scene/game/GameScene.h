#pragma once

#include "../interface/BaseScene.h"
#include "BattleController.h"

namespace Bonjin {

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
	

private:
	std::unique_ptr<BattleController> battleController_;
};

}
