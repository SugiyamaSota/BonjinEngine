#pragma once

#include <Windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <wrl/client.h>
#include <XInput.h>

class Input {
private:
    Input() = default;
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    // キー入力
    Microsoft::WRL::ComPtr<IDirectInput8> directInput_;
    Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_;
    BYTE keyState_[256];
    BYTE prevKeyState_[256];

    // マウス入力
    Microsoft::WRL::ComPtr<IDirectInputDevice8> mouse_; // マウスデバイス
    DIMOUSESTATE2 mouseState_; // 現在のマウス状態
    DIMOUSESTATE2 prevMouseState_; // 1フレーム前のマウス状態

    // ゲームパッド入力
    XINPUT_STATE gamepadState_{};
    XINPUT_STATE prevGamepadState_{};
    bool isGamepadConnected_ = false;

    // マウス固定関連
    bool isMouseLocked_ = false;
    HWND hwnd_ = nullptr;

public:
    /// --- 入力クラス全般 ---
    // シングルトンインスタンスを取得
    static Input* GetInstance();
    // 初期化
    void Initialize(HINSTANCE hInstance, HWND hwnd);
    // 更新
    void Update();

    /// --- キー入力 ---
    // キーが押されているか
    bool IsPress(int DIK_KEY);

    // キーがトリガー（押された瞬間）か
    bool IsTrigger(int DIK_KEY);

    /// --- マウス入力 ---
    // マウスボタンが押されているか
    bool IsMousePress(int button); // button は DIMOFS_BUTTON0 など

    // マウスボタンがトリガー（押された瞬間）か
    bool IsMouseTrigger(int button);

    // マウスのX軸方向の移動量を取得
    long GetMouseDeltaX();

    // マウスのY軸方向の移動量を取得
    long GetMouseDeltaY();

    // マウスのホイール移動量を取得
    long GetMouseWheel();

    // マウスの固定状態の変更
    void SetMouseLock(bool lock);

    /// --- ゲームパッド入力 ---
    // パッドのボタンが押されているか
    bool IsPadPress(int button);

    // パッドのボタンがトリガー（押された瞬間）か
    bool IsPadTrigger(int button);

    // パッドの左スティックのx軸を取得
    long GetPadLStickX();

    // パッドの左スティックのy軸を取得
    long GetPadLStickY();

    // パッドの右スティックのx軸を取得
    long GetPadRStickX();

    // パッドの右スティックのy軸を取得
    long GetPadRStickY();

    // パッドの十字キーの方向を取得 (DIK_*キーと同様の値を返す)
    long GetPadPov();

    // ゲームパッドが接続されているか
    bool IsPadConnected();
};