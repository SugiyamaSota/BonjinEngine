#include "SceneTransition.h"
#include "SceneManager.h"
#include <algorithm>

using namespace Bonjin;

void SceneTransition::Initialize() {
    Reset();
}

void SceneTransition::Reset() {
    CleanupEffects();
    phase_ = TransitionPhase::kIdle;
    timer_ = 0.0f;
    progress_ = 0.0f;
    shouldSwitchScene_ = false;
}

void SceneTransition::Start(const TransitionDesc& desc) {
    desc_ = desc;
    timer_ = 0.0f;
    progress_ = 0.0f;
    shouldSwitchScene_ = false;

    // 即時切り替え（演出なし）の場合
    if (desc_.outType == TransitionType::kNone && desc_.inType == TransitionType::kNone) {
        shouldSwitchScene_ = true;
        phase_ = TransitionPhase::kIdle;
        return;
    }

    // アウト演出がNoneの場合は直ちに待機/切り替えフェーズへ
    if (desc_.outType == TransitionType::kNone) {
        EnterPhase(TransitionPhase::kWait);
    } else {
        EnterPhase(TransitionPhase::kOut);
    }
}

void SceneTransition::EnterPhase(TransitionPhase nextPhase) {
    phase_ = nextPhase;
    timer_ = 0.0f;
    progress_ = 0.0f;

    auto sm = SceneManager::GetInstance();

    switch (phase_) {
    case TransitionPhase::kOut:
        // アウト演出の開始準備
        if (desc_.outType == TransitionType::kDigitalDissolve || desc_.outType == TransitionType::kFadeBlack) {
            sm->AddPostEffect(DirectXCommon::PostEffect::kDigitalDissolve);
            sm->SetDissolveThreshold(1.0f); // 完全に表示された状態から開始
            sm->SetDissolveEdgeWidth(desc_.outType == TransitionType::kFadeBlack ? 0.0f : 0.06f);
            sm->SetDissolveEdgeColor(desc_.edgeColor);
        } else if (desc_.outType == TransitionType::kCRTShutDown) {
            sm->AddPostEffect(DirectXCommon::PostEffect::kCRTShutDown);
            sm->SetCRTShutDownProgress(0.0f);
        }
        break;

    case TransitionPhase::kWait:
        // 暗転状態の維持・シーン切り替え指示
        shouldSwitchScene_ = true;

        if (desc_.outType == TransitionType::kDigitalDissolve || desc_.outType == TransitionType::kFadeBlack) {
            sm->SetDissolveThreshold(0.0f); // 完全暗転
        } else if (desc_.outType == TransitionType::kCRTShutDown) {
            sm->SetCRTShutDownProgress(1.0f); // 完全ブラックアウト
        }
        break;

    case TransitionPhase::kIn:
        // 前のアウト演出固有のエフェクトをクリーンアップ
        if (desc_.outType == TransitionType::kCRTShutDown && desc_.inType != TransitionType::kCRTShutDown) {
            sm->SetCRTShutDownProgress(0.0f);
            sm->RemovePostEffect(DirectXCommon::PostEffect::kCRTShutDown);
        }

        // イン演出の開始準備
        if (desc_.inType == TransitionType::kDigitalDissolve || desc_.inType == TransitionType::kFadeBlack) {
            sm->AddPostEffect(DirectXCommon::PostEffect::kDigitalDissolve);
            sm->SetDissolveThreshold(0.0f); // 真っ黒から実体化スタート
            sm->SetDissolveEdgeWidth(desc_.inType == TransitionType::kFadeBlack ? 0.0f : 0.06f);
            sm->SetDissolveEdgeColor(desc_.edgeColor);
        }
        break;

    case TransitionPhase::kIdle:
        CleanupEffects();
        break;
    }

    ApplyCurrentEffect();
}

void SceneTransition::Update(float deltaTime) {
    if (phase_ == TransitionPhase::kIdle) {
        return;
    }

    timer_ += deltaTime;

    switch (phase_) {
    case TransitionPhase::kOut: {
        float dur = (desc_.outDuration <= 0.0f) ? 0.001f : desc_.outDuration;
        progress_ = std::clamp(timer_ / dur, 0.0f, 1.0f);
        ApplyCurrentEffect();

        if (progress_ >= 1.0f) {
            EnterPhase(TransitionPhase::kWait);
        }
        break;
    }

    case TransitionPhase::kWait: {
        float dur = desc_.waitDuration;
        if (dur <= 0.0f || timer_ >= dur) {
            if (desc_.inType == TransitionType::kNone) {
                EnterPhase(TransitionPhase::kIdle);
            } else {
                EnterPhase(TransitionPhase::kIn);
            }
        }
        break;
    }

    case TransitionPhase::kIn: {
        float dur = (desc_.inDuration <= 0.0f) ? 0.001f : desc_.inDuration;
        progress_ = std::clamp(timer_ / dur, 0.0f, 1.0f);
        ApplyCurrentEffect();

        if (progress_ >= 1.0f) {
            EnterPhase(TransitionPhase::kIdle);
        }
        break;
    }

    case TransitionPhase::kIdle:
        break;
    }
}

void SceneTransition::ApplyCurrentEffect() {
    auto sm = SceneManager::GetInstance();

    if (phase_ == TransitionPhase::kOut) {
        if (desc_.outType == TransitionType::kDigitalDissolve || desc_.outType == TransitionType::kFadeBlack) {
            // 表示(1.0) -> 消失(0.0)
            sm->SetDissolveThreshold(1.0f - progress_);
        } else if (desc_.outType == TransitionType::kCRTShutDown) {
            // 0.0 -> 1.0
            sm->SetCRTShutDownProgress(progress_);
        }
    } else if (phase_ == TransitionPhase::kIn) {
        if (desc_.inType == TransitionType::kDigitalDissolve || desc_.inType == TransitionType::kFadeBlack) {
            // 消失(0.0) -> 表示(1.0)
            sm->SetDissolveThreshold(progress_);
        }
    }
}

void SceneTransition::CleanupEffects() {
    auto sm = SceneManager::GetInstance();
    sm->RemovePostEffect(DirectXCommon::PostEffect::kDigitalDissolve);
    sm->RemovePostEffect(DirectXCommon::PostEffect::kCRTShutDown);
    sm->SetCRTShutDownProgress(0.0f);
    sm->SetDissolveThreshold(1.0f);
}
