#pragma once
#include "../interface/BaseScene.h" // ISceneをインクルード

#include "Animation.h"
#include "Skeleton.h"
#include "Object3D.h"
#include "SkeletonDebugRenderer.h"
#include "SkyBox.h"
#include "Sprite.h"
#include"ParticleManager.h"

namespace Bonjin
{

	enum class TestPhase {
		kStart,
		kPlay,
		kGoal,
	};

	class TestScene : public BaseScene<TestPhase>
	{
	public:
		// --- オーバーライド関数 --- 
		virtual ~TestScene() = default;

		void Initialize(Camera* camera) override;

		void Unload()override;

		void Update(float deltaTime) override;

		void Draw() override;

		SceneType GetNextScene() const override;

		void DrawSceneImGui() override;

		const char* GetScenename()const override {
			return "TestScene";
		}

	private:
		// --- ゲーム固有の変数 ---
		// テストモデルspher
		std::unique_ptr<Object3D> testModel_ = nullptr;

		// 
		std::unique_ptr<SkyBox> testSkyBox_ = nullptr;

		// テストモデルcube
		std::unique_ptr<Object3D> testCube_ = nullptr;
		Animation cubeAnimation_{};
		Skeleton cubeSkeleton_{};
		std::unique_ptr<SkeletonDebugRenderer> skeletonDebugRenderer_ = nullptr;
		Matrix4x4 animatedModelWorldMatrix_{};
		float animationTime_ = 0.0f;

		std::unique_ptr<Sprite> testSprite_ = nullptr;

		ParticleManager* pm = nullptr;
		ParticleConfig ringEffect;

	private:
		// --- ゲーム固有の関数 ---
	};
}
