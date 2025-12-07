#pragma once
#include "../base/SceneBase.h"

#include"../../field/Skydome.h"

// シーンの状態を表す列挙型
enum class TitlePhase {
    kFadeIn,
    kActive,
    kFadeOut,
};

namespace BonjinEngine {

    class TitleScene : public SceneBase {
    public:
        // 💡 仮想デストラクタは必須
        virtual ~TitleScene();

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
        //
        bool isFinished_;

        // フェーズ関連
        TitlePhase phase_ = TitlePhase::kFadeIn;
        float phaseTimer_ = 0.0f;

        Model* titleModel_ = nullptr;
        WorldTransform worldTransform_;

        Model* titleUIModel_ = nullptr;

        // --- 天球関連 ---
        std::unique_ptr<Skydome> skydome_ = nullptr;
        std::unique_ptr<Model> skydomeModel_ = nullptr;

        // UI
        Sprite* blackScreenSprite_ = nullptr;

    };
}