#include "Time.h"
#include <iostream> // デバッグ用など (必要に応じて)

using namespace Bonjin;
using namespace std::chrono;

// 1. シングルトンインスタンスの実体
Time* Time::instance = nullptr;

// 2. シングルトン: インスタンスの取得と初期化
Time* Time::GetInstance() {
    if (instance == nullptr) {
        instance = new Time();
        // 💡 最初のフレームの基準時間を設定
        instance->prevTime_ = steady_clock::now();
    }
    return instance;
}

// 3. シングルトン: インスタンスの破棄
void Time::DestroyInstance() {
    delete instance;
    instance = nullptr;
}

// 4. 毎フレームの更新処理
void Time::Update() {
    // 1. 現在時刻を取得
    steady_clock::time_point currentTime = steady_clock::now();

    // 2. 差分を計算 (duration<float>で秒単位の浮動小数点数にする)
    duration<float> timeSpan = currentTime - prevTime_;

    // 3. デルタタイムを更新
    deltaTime_ = timeSpan.count();

    // 4. 経過時間を更新
    elapsedTime_ += deltaTime_;

    // 5. 次のフレームのために現在時刻を保存
    prevTime_ = currentTime;

    // 💡 (オプション) デルタタイムに上限を設ける
    // (例: フレームレートが極端に落ちた場合に物理演算が破綻しないように)
    const float kMaxDeltaTime = 0.1f; // 10 FPS 相当
    if (deltaTime_ > kMaxDeltaTime) {
        deltaTime_ = kMaxDeltaTime;
    }
}