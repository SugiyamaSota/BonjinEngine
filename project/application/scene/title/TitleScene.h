#pragma once
#include "../interface/BaseScene.h"
#include "../bonjin/BonjinEngine.h"
#include "TitleWorld.h"
#include "TitleUI.h"
#include <memory>

namespace Bonjin
{

    enum class TitlePhase {
        kFadeIn,
        kActive,
        kDive,         // ズーム＆激震シェイク突入（約1.0秒）
        kShutDown,     // ポストエフェクトによる「プツン」電源断演出（約0.22秒）
        kBlackout,     // 漆黒の静寂・パチンコ風ブラックアウト（約0.50秒）
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

    private:
        // --- 3D背景ワールド ---
        std::unique_ptr<TitleWorld> world_ = nullptr;

        // --- 2D UI / HUD ---
        std::unique_ptr<TitleUI> ui_ = nullptr;

        // --- タイマー・演出制御 ---
        float timer_ = 0.0f;
        float diveTimer_ = 0.0f;
        const float kDiveDuration = 1.0f; // ダイブ（ズーム＆シェイク）にかける時間

        float shutDownTimer_ = 0.0f;
        const float kShutDownDuration = 0.24f; // ポストエフェクト「プツン」電源断の時間

        float blackoutTimer_ = 0.0f;
        const float kBlackoutDuration = 0.50f; // 完全暗転（ブラックアウト静寂）の時間

        SceneType targetSceneType_ = "GameScene"; // 遷移先シーン（GameScene または TutorialScene）

    private:
        void UpdateDiveTransition(float deltaTime);
        void UpdateShutDownTransition(float deltaTime);
        void UpdateBlackout(float deltaTime);
    };

}