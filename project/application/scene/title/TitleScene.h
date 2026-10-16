#pragma once
#include "../interface/BaseScene.h"
#include "../bonjin/BonjinEngine.h"
#include "SoundManager.h"

#include "TitleWorld.h"
#include "TitleUI.h"

#include <memory>

namespace Bonjin
{

    enum class TitlePhase {
        kActive,
        kDive,         // ズーム＆激震シェイク突入（約1.0秒）
    };

    class TitleScene : public BaseScene<TitlePhase>
    {
    public:
        virtual ~TitleScene() = default;

        void Initialize(Camera* camera) override;

        void Unload() override;

        void Update(float deltaTime) override;

        void Draw() override;
        void DrawImGui() override;

        SceneType GetNextScene() const override;

        const char* GetScenename() const override
        {
            return "TitleScene";
        }

        TransitionType GetTransitionOutType() const override { return TransitionType::kCRTShutDown; }
        float GetTransitionOutDuration() const override { return 0.24f; }
        float GetTransitionWaitDuration() const override { return 0.50f; } // パチンコ風の息を呑む静寂のタメ（ブラックアウト）

    private:
        // --- 3D背景ワールド ---
        std::unique_ptr<TitleWorld> world_ = nullptr;

        // --- 2D UI / HUD ---
        std::unique_ptr<TitleUI> ui_ = nullptr;

        // --- タイマー・演出制御 ---
        float timer_ = 0.0f;
        float diveTimer_ = 0.0f;
        const float kDiveDuration = 1.0f; // ダイブ（ズーム＆シェイク）にかける時間

        SceneType targetSceneType_ = "GameScene"; // 遷移先シーン（GameScene または TutorialScene）

    private:
        void UpdateDiveTransition(float deltaTime);
    };

}