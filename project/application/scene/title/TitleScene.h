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
        kFadeOut,
    };

    class TitleScene : public BaseScene<TitlePhase>
    {
    public:
        virtual ~TitleScene() = default;

        void Initialize(Camera* camera) override;

        void Unload() override;

        void Update(float deltaTime) override;

        void Draw() override;

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
        const float kDiveDuration = 1.3f; // ダイブ遷移にかける時間（秒）

    private:
        void UpdateDiveTransition(float deltaTime);
    };

}