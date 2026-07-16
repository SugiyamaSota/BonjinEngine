#pragma once

#include <Windows.h>
#include <XInput.h>
#include <cstdint>

class Gamepad {
private:
    Gamepad() = default;
    Gamepad(const Gamepad&) = delete;
    Gamepad& operator=(const Gamepad&) = delete;

    static constexpr uint32_t kMaxGamepadCount = 4;
    XINPUT_STATE gamepadStates_[kMaxGamepadCount]{};
    XINPUT_STATE prevGamepadStates_[kMaxGamepadCount]{};
    bool isGamepadConnected_[kMaxGamepadCount]{};

public:
    // シングルトンインスタンスを取得
    static Gamepad* GetInstance();

    // 更新
    void Update();

    // ボタンが押されているか
    bool IsPress(int button, uint32_t userIndex = 0);

    // ボタンがトリガー（押された瞬間）か
    bool IsTrigger(int button, uint32_t userIndex = 0);

    // 左スティックのx軸を取得
    long GetLStickX(uint32_t userIndex = 0);

    // 左スティックのy軸を取得
    long GetLStickY(uint32_t userIndex = 0);

    // 右スティックのx軸を取得
    long GetRStickX(uint32_t userIndex = 0);

    // 右スティックのy軸を取得
    long GetRStickY(uint32_t userIndex = 0);

    // 十字キーの状態を取得 (XINPUT_GAMEPAD_DPAD_* のマスク値を返す)
    long GetPov(uint32_t userIndex = 0);

    // ゲームパッドが接続されているか
    bool IsConnected(uint32_t userIndex = 0);
};
