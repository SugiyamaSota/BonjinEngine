#include "Gamepad.h"
#include <cassert>
#include <memory>
#include <cmath>

#pragma comment(lib, "xinput.lib")

static std::unique_ptr<Gamepad> sGamepadInstance;

Gamepad* Gamepad::GetInstance() {
    if (!sGamepadInstance) {
        sGamepadInstance = std::unique_ptr<Gamepad>(new Gamepad());
    }
    return sGamepadInstance.get();
}

void Gamepad::Update() {
    // 0~3 番のすべてのスロットのコントローラー状態をポーリング
    for (DWORD i = 0; i < kMaxGamepadCount; ++i) {
        prevGamepadStates_[i] = gamepadStates_[i];
        ZeroMemory(&gamepadStates_[i], sizeof(XINPUT_STATE));
        DWORD dwResult = XInputGetState(i, &gamepadStates_[i]);
        isGamepadConnected_[i] = (dwResult == ERROR_SUCCESS);
    }
}

bool Gamepad::IsPress(int button, uint32_t userIndex) {
    if (userIndex >= kMaxGamepadCount) return false;
    if (!isGamepadConnected_[userIndex]) return false;
    return (gamepadStates_[userIndex].Gamepad.wButtons & button) != 0;
}

bool Gamepad::IsTrigger(int button, uint32_t userIndex) {
    if (userIndex >= kMaxGamepadCount) return false;
    if (!isGamepadConnected_[userIndex]) return false;
    return ((gamepadStates_[userIndex].Gamepad.wButtons & button) != 0) &&
           ((prevGamepadStates_[userIndex].Gamepad.wButtons & button) == 0);
}

long Gamepad::GetLStickX(uint32_t userIndex) {
    if (userIndex >= kMaxGamepadCount) return 0;
    if (!isGamepadConnected_[userIndex]) return 0;
    SHORT val = gamepadStates_[userIndex].Gamepad.sThumbLX;
    if (abs(val) < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) return 0;
    return val;
}

long Gamepad::GetLStickY(uint32_t userIndex) {
    if (userIndex >= kMaxGamepadCount) return 0;
    if (!isGamepadConnected_[userIndex]) return 0;
    SHORT val = gamepadStates_[userIndex].Gamepad.sThumbLY;
    if (abs(val) < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) return 0;
    return val;
}

long Gamepad::GetRStickX(uint32_t userIndex) {
    if (userIndex >= kMaxGamepadCount) return 0;
    if (!isGamepadConnected_[userIndex]) return 0;
    SHORT val = gamepadStates_[userIndex].Gamepad.sThumbRX;
    if (abs(val) < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) return 0;
    return val;
}

long Gamepad::GetRStickY(uint32_t userIndex) {
    if (userIndex >= kMaxGamepadCount) return 0;
    if (!isGamepadConnected_[userIndex]) return 0;
    SHORT val = gamepadStates_[userIndex].Gamepad.sThumbRY;
    if (abs(val) < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) return 0;
    return val;
}

long Gamepad::GetPov(uint32_t userIndex) {
    if (userIndex >= kMaxGamepadCount) return 0;
    if (!isGamepadConnected_[userIndex]) return 0;
    return gamepadStates_[userIndex].Gamepad.wButtons & 
           (XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT);
}

bool Gamepad::IsConnected(uint32_t userIndex) {
    if (userIndex >= kMaxGamepadCount) return false;
    return isGamepadConnected_[userIndex];
}
