#pragma once
#include <chrono>

namespace Bonjin {

    class Time {
    public:
        // シングルトン: インスタンスの取得
        static Time* GetInstance();
        // シングルトン: インスタンスの破棄
        static void DestroyInstance();

    public:
        // 💡 毎フレームの開始時に呼び出し、次のフレームのデルタタイムを計算するための準備をする
        void Update();

        // 💡 デルタタイムを取得 (秒単位)
        float GetDeltaTime() const { return deltaTime_; }

        // 💡 経過時間 (起動からの合計時間) を取得 (秒単位)
        float GetElapsedTime() const { return elapsedTime_; }

    private:
        static Time* instance;

        // シングルトン関連の禁止
        Time() = default;
        ~Time() = default;
        // ... (コピー禁止など)

        // 💡 デルタタイム (秒)
        float deltaTime_ = 0.0f;
        // 💡 経過時間 (秒)
        float elapsedTime_ = 0.0f;

        // 💡 前のフレームの時刻
        std::chrono::steady_clock::time_point prevTime_;
    };

}