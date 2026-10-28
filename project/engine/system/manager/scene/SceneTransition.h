#pragma once
#include <string>
#include "math/Struct.h"
#include "PostEffectTypes.h"

namespace Bonjin {

    /// <summary>
    /// シーン遷移演出の種類
    /// </summary>
    enum class TransitionType {
        kNone,              // 即座に切り替え
        kDigitalDissolve,   // 電子ブロック・デジタルディゾルブ
        kCRTShutDown,       // CRTモニター電源断（プツン）
        kFadeBlack,         // 単純な暗転（SimpleFade）
        kSimpleFade = kFadeBlack, // SimpleFade エイリアス
        kCustom,            // シーン固有のカスタム演出に委任
    };

    /// <summary>
    /// トランジションのフェーズ
    /// </summary>
    enum class TransitionPhase {
        kIdle,  // 通常時（遷移なし）
        kOut,   // 画面消失演出（アウト演出）
        kWait,  // 暗転中のタメ・静寂（ロード/シーン切替タイミング）
        kIn,    // 画面出現演出（イン演出）
    };

    /// <summary>
    /// シーン遷移演出の設定記述構造体
    /// </summary>
    struct TransitionDesc {
        TransitionType outType = TransitionType::kDigitalDissolve; // アウト演出の種類
        float outDuration = 0.8f;                                  // アウト演出の秒数
        float waitDuration = 0.2f;                                 // 暗転中のタメ秒数
        TransitionType inType = TransitionType::kDigitalDissolve;  // イン演出の種類
        float inDuration = 1.0f;                                   // イン演出の秒数
        Vector3 edgeColor = { 0.0f, 1.5f, 2.0f };                  // ディゾルブ時のサイバー発光色
        DissolveMode dissolveMode = DissolveMode::kBottomToTop;     // ディゾルブの方向・モード
    };

    /// <summary>
    /// シーン遷移演出クラス
    /// </summary>
    class SceneTransition {
    public:
        SceneTransition() = default;
        ~SceneTransition() = default;

        void Initialize();
        void Update(float deltaTime);

        /// <summary>
        /// トランジションシーケンスの開始
        /// </summary>
        void Start(const TransitionDesc& desc);

        /// <summary>
        /// トランジション状態の取得
        /// </summary>
        TransitionPhase GetPhase() const { return phase_; }
        bool IsActive() const { return phase_ != TransitionPhase::kIdle; }

        /// <summary>
        /// シーン切り替え（Unload & Initialize）を実行すべきタイミングか
        /// </summary>
        bool ShouldSwitchScene() const { return shouldSwitchScene_; }
        void NotifySceneSwitched() { shouldSwitchScene_ = false; }

        /// <summary>
        /// トランジションを強制リセット
        /// </summary>
        void Reset();

        /// <summary>
        /// 現在の進行度（0.0f ～ 1.0f）
        /// </summary>
        float GetProgress() const { return progress_; }

    private:
        TransitionPhase phase_ = TransitionPhase::kIdle;
        TransitionDesc desc_;
        float timer_ = 0.0f;
        float progress_ = 0.0f;
        bool shouldSwitchScene_ = false;

    private:
        void EnterPhase(TransitionPhase nextPhase);
        void ApplyCurrentEffect();
        void CleanupEffects();
        void AddPostEffectOnce(PostEffect effect); // 未登録の場合のみ追加（重複防止）
    };

}
