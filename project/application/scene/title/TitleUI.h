#pragma once
#include "TextSprite.h"
#include <memory>

namespace Bonjin
{

    /// <summary>
    /// タイトル画面の2D UI・HUDテキスト・グリッチ演出を管理するクラス
    /// </summary>
    class TitleUI
    {
    public:
        TitleUI() = default;
        ~TitleUI() = default;

        /// <summary>
        /// 初期化
        /// </summary>
        void Initialize();

        /// <summary>
        /// 解放
        /// </summary>
        void Unload();

        /// <summary>
        /// 更新処理
        /// </summary>
        /// <param name="deltaTime">経過時間</param>
        /// <param name="isDiving">ダイブ遷移中フラグ</param>
        /// <param name="diveProgress">ダイブ進行度 (0.0f ~ 1.0f)</param>
        void Update(float deltaTime, bool isDiving = false, float diveProgress = 0.0f);

        /// <summary>
        /// 描画処理
        /// </summary>
        void Draw();

        /// <summary>
        /// 全体UIの不透明度を設定
        /// </summary>
        void SetAlpha(float alpha);

    private:
        // 2Dテキストスプライト
        std::unique_ptr<TextSprite> titleText_ = nullptr;
        std::unique_ptr<TextSprite> subTitleText_ = nullptr;
        std::unique_ptr<TextSprite> systemText_ = nullptr;
        std::unique_ptr<TextSprite> startText_ = nullptr;

        // タイマー・演出パラメータ
        float timer_ = 0.0f;
        float glitchTimer_ = 0.0f;
        bool isGlitching_ = false;

    private:
        void UpdateGlitch(float deltaTime);
    };

}
