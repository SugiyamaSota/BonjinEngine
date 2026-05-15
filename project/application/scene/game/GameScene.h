#pragma once
#include "../interface/BaseScene.h" // ISceneをインクルード

#include "Object3D.h"

#include"ParticleManager.h"

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
		std::unique_ptr<Object3D> testModel_ = nullptr;

		std::unique_ptr<Object3D> testSkyBox_ = nullptr;

		ParticleManager* pm = nullptr;
		ParticleConfig ringEffect;

	private:
		// --- ゲーム固有の関数 ---
	};
}