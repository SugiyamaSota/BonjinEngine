#pragma once

#include "math/Struct.h"

enum class Color : uint32_t {
    White = 0xFFFFFFFF,
    Black = 0x000000FF,
    Red = 0xFF0000FF,
    Green = 0x00FF00FF,
    Blue = 0x0000FFFF,
    Yellow = 0xFFFF00FF,
    Cyan = 0x00FFFFFF,
    Magenta = 0xFF00FFFF,
    Transparent = 0x00000000,
    Favorite = 0xDDA0DDFF,
};

// enum Color から Vector4 に変換するユーティリティ関数
inline Vector4 ToVector4(Color color) {
    uint32_t colorValue = static_cast<uint32_t>(color);

    float r = ((colorValue >> 24) & 0xFF) / 255.0f;
    float g = ((colorValue >> 16) & 0xFF) / 255.0f;
    float b = ((colorValue >> 8) & 0xFF) / 255.0f;
    float a = (colorValue & 0xFF) / 255.0f;

    return Vector4(r, g, b, a);
}

//imgui::comboで使うための配列
namespace ColorUtils {
    const std::vector<std::pair<const char*, Color>> ColorList = {
        {"White", Color::White},
        {"Black", Color::Black},
        {"Red", Color::Red},
        {"Green", Color::Green},
        {"Blue", Color::Blue},
        {"Yellow", Color::Yellow},
        {"Cyan", Color::Cyan},
        {"Magenta", Color::Magenta},
        {"Transparent", Color::Transparent},
        {"Favorite (Light Purple)", Color::Favorite},
    };
}