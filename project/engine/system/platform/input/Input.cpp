#include "Input.h"
#include <cassert>
#include <memory> // std::unique_ptr を使用するために追加

#pragma comment(lib,"dinput8.lib")

// シングルトンインスタンスを保持する静的ポインタ
// スレッドセーフなシングルトンのために std::call_once や std::mutex を使用することも検討できますが、
// ここではシンプルな実装としています。
static std::unique_ptr<Input> sInputInstance;

Input* Input::GetInstance() {
	if (!sInputInstance) {
		sInputInstance = std::unique_ptr<Input>(new Input());
	}
	return sInputInstance.get();
}

void Input::Initialize(HINSTANCE hInstance, HWND hwnd) {
	HRESULT hr = DirectInput8Create(
		hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void**)directInput_.GetAddressOf(), nullptr);
	assert(SUCCEEDED(hr));

    // hwndを保持
    this->hwnd_ = hwnd;

	// キーボードデバイスの取得
	hr = directInput_->CreateDevice(GUID_SysKeyboard, keyboard_.GetAddressOf(), NULL);
	assert(SUCCEEDED(hr));

	hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(hr));

	hr = keyboard_->SetCooperativeLevel(
		hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(hr));

	// マウスデバイスの生成
	hr = directInput_->CreateDevice(GUID_SysMouse, &mouse_, NULL);
	assert(SUCCEEDED(hr));

	hr = mouse_->SetDataFormat(&c_dfDIMouse2);
	assert(SUCCEEDED(hr));

	hr = mouse_->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	assert(SUCCEEDED(hr));

	mouse_->Acquire();
    // XInput は動的に検出するため初期化時の列挙処理は不要です
}


void Input::Update() {
    // 前のフレームのキー状態を保存
    memcpy(prevKeyState_, keyState_, sizeof(keyState_));

    // 現在のキー状態を取得
    // キーボードも同様にエラーハンドリングを強化することを推奨します
    keyboard_->Acquire(); // キーボードもAcquire()が必要
    HRESULT hrKey = keyboard_->GetDeviceState(sizeof(keyState_), keyState_);
    if (FAILED(hrKey)) {
        if ((hrKey == DIERR_INPUTLOST) || (hrKey == DIERR_NOTACQUIRED)) {
            // キーボードの再取得も試みる
            keyboard_->Acquire();
        }
        ZeroMemory(&keyState_, sizeof(keyState_)); // エラー時は状態をクリア
    }

    // マウスの状態を更新
    prevMouseState_ = mouseState_; // 前のフレームの状態を保存

    HRESULT hrMouse = mouse_->GetDeviceState(sizeof(mouseState_), &mouseState_);
    if (FAILED(hrMouse)) {
        // デバイスがロストした場合などに対応
        if ((hrMouse == DIERR_INPUTLOST) || (hrMouse == DIERR_NOTACQUIRED)) {
            // ここでAcquireを呼び出すのが正しいパターン
            HRESULT hrAcquire = mouse_->Acquire();
            // Acquireが成功したかどうかも確認する（S_OK または S_FALSE は成功とみなす）
            if (FAILED(hrAcquire)) {
                // Acquire自体が失敗した場合、エラーログを出力するなど
                char buffer[64];
                sprintf_s(buffer, sizeof(buffer), "Failed to re-acquire mouse: HRESULT = 0x%X\n", hrAcquire);
                OutputDebugStringA(buffer);
            }
        }
        // それ以外のエラー、または再取得後もまだ状態が取得できない場合は状態をクリア
        ZeroMemory(&mouseState_, sizeof(mouseState_));
        // デバッグ用にエラーコードを出力
        char buffer[64];
        sprintf_s(buffer, sizeof(buffer), "Mouse GetDeviceState failed: HRESULT = 0x%X\n", hrMouse);
        OutputDebugStringA(buffer);
        // ここでreturnしてもよいし、このまま継続して次のフレームで再試行でもよい
        // 今回はとりあえず状態をクリアして続行
    }


	// ゲームパッドの状態を更新（全4台分ループ）


    // マウス固定状態の処理
    if (isMouseLocked_) {
        RECT clientRect;
        GetClientRect(hwnd_, &clientRect);

        // クライアント座標をスクリーン座標に変換
        POINT clientTopLeft = { clientRect.left, clientRect.top };
        POINT clientBottomRight = { clientRect.right, clientRect.bottom };
        ClientToScreen(hwnd_, &clientTopLeft);
        ClientToScreen(hwnd_, &clientBottomRight);

        // 中央座標を計算（スクリーン座標）
        int centerX = (clientTopLeft.x + clientBottomRight.x) / 2;
        int centerY = (clientTopLeft.y + clientBottomRight.y) / 2;

        // 強制的に中央に戻す
        SetCursorPos(centerX, centerY);
    }
}

bool Input::IsPress(int DIK_KEY) {
	return (keyState_[DIK_KEY] & 0x80);
}

bool Input::IsTrigger(int DIK_KEY) {
	return (keyState_[DIK_KEY] & 0x80) && !(prevKeyState_[DIK_KEY] & 0x80);
}

bool Input::IsMousePress(int button) {
	return (mouseState_.rgbButtons[button] & 0x80) != 0;
}

bool Input::IsMouseTrigger(int button) {
	return (mouseState_.rgbButtons[button] & 0x80) != 0 &&
		(prevMouseState_.rgbButtons[button] & 0x80) == 0;
}

long Input::GetMouseDeltaX() {
	return mouseState_.lX;
}

long Input::GetMouseDeltaY() {
	return mouseState_.lY;
}

long Input::GetMouseWheel() {
	return mouseState_.lZ;
}

void Input::SetMouseLock(bool lock) {
    if (isMouseLocked_ == lock) {
        return;
    }
    isMouseLocked_ = lock;

    if (isMouseLocked_) {
        // 1. カーソルを非表示
        ShowCursor(FALSE);

        // 2. カーソルをウィンドウ内に制限 (ClipCursor)
        RECT clientRect;
        // クライアント領域の座標を取得
        GetClientRect(hwnd_, &clientRect);

        // クライアント座標をスクリーン座標に変換 (ClipCursorはスクリーン座標を要求するため)
        POINT clientTopLeft = { clientRect.left, clientRect.top };
        POINT clientBottomRight = { clientRect.right, clientRect.bottom };
        ClientToScreen(hwnd_, &clientTopLeft);
        ClientToScreen(hwnd_, &clientBottomRight);

        RECT clipRect;
        clipRect.left = clientTopLeft.x;
        clipRect.top = clientTopLeft.y;
        clipRect.right = clientBottomRight.x;
        clipRect.bottom = clientBottomRight.y;

        // マウス入力をこの領域内に制限
        ClipCursor(&clipRect);

        // 3. カーソルを中央にリセット（固定開始時）
        int centerX = (clipRect.left + clipRect.right) / 2;
        int centerY = (clipRect.top + clipRect.bottom) / 2;
        SetCursorPos(centerX, centerY);

    } else {
        // 1. カーソルを再表示
        ShowCursor(TRUE);

        // 2. カーソル制限を解除
        ClipCursor(NULL);
    }
}
