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

    // 仮想入力 (デモの自動操作用)。有効な間は本物のキーを無視し、SetVirtualKeyで押したキーだけが見える
    bool isVirtualMode_ = false;

    // マウス入力
    Microsoft::WRL::ComPtr<IDirectInputDevice8> mouse_; // マウスデバイス
    DIMOUSESTATE2 mouseState_; // 現在のマウス状態
    DIMOUSESTATE2 prevMouseState_; // 1フレーム前のマウス状態



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

    /// --- 仮想入力 (デモの自動操作用) ---
    void SetVirtualMode(bool enabled);
    bool IsVirtualMode() const { return isVirtualMode_; }
    // 今のフレームの仮想キーを設定する (Updateの後に呼ぶ。毎フレーム離した状態に戻る)
    void SetVirtualKey(int DIK_KEY, bool pressed);
};