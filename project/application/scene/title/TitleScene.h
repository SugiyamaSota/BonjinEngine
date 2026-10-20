#pragma once
#include "../interface/BaseScene.h"
#include "../bonjin/BonjinEngine.h"

#include "TitleWorld.h"
#include "TitleUI.h"

#include <memory>

namespace Bonjin
{

    enum class TitlePhase {
        kActive,
        kDive,         // ズーム＆激震シェイク突入（約1.0秒）
    };

    class TitleScene : public BaseScene<TitleScene>
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

        // --- フェーズから参照するアクセサ ---
        TitleWorld* GetWorld() const { return world_.get(); }
        TitleUI* GetUI() const { return ui_.get(); }
        Camera* GetCamera() const { return camera_; }
        uint32_t GetShutdownSEId() const { return shutdownSEId; }

    private:
        // --- 3D背景ワールド ---
        std::unique_ptr<TitleWorld> world_ = nullptr;

        // --- 2D UI / HUD ---
        std::unique_ptr<TitleUI> ui_ = nullptr;

        // --- タイマー ---
        float timer_ = 0.0f;

        uint32_t shutdownSEId = 0;
    };

}