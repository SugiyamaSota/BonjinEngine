#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <windows.h>
#include <xaudio2.h>

/// <summary>
/// 音声データ構造体
/// </summary>
struct SoundData {
    WAVEFORMATEXTENSIBLE format = {};
    std::vector<BYTE> buffer;
    std::string filePath;
    uint32_t id = 0;
};
