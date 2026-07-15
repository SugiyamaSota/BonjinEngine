#include "Sprite.h"
#include "TextureManager.h"
#include <cassert>

namespace Bonjin {

    Sprite::Sprite() : BaseSprite()
    {
    }

    Sprite::~Sprite()
    {
    }

    void Sprite::Initialize(const std::string& textureFilePath) {
        BaseSprite::Initialize();

        // テクスチャロード
        textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/textures/" + textureFilePath);

        // 初回の頂点情報を構築
        RefreshVertexData();
    }

    D3D12_GPU_DESCRIPTOR_HANDLE Sprite::GetSRVHandle() const {
        return TextureManager::GetInstance()->GetGPUHandle(textureHandle_);
    }

    void Sprite::RefreshVertexData() {
        if (!vertexData_) return;

        // テクスチャサイズの取得 (切り取り計算用)
        const D3D12_RESOURCE_DESC& resDesc = TextureManager::GetInstance()->GetResourceDesc(textureHandle_);
        float texW = static_cast<float>(resDesc.Width);
        float texH = static_cast<float>(resDesc.Height);

        // UVの決定
        float u_left = 0.0f, u_right = 1.0f, v_top = 0.0f, v_bottom = 1.0f;
        if (textureRect_.z != 0 && textureRect_.w != 0) {
            u_left = textureRect_.x / texW;
            v_top = textureRect_.y / texH;
            u_right = (textureRect_.x + textureRect_.z) / texW;
            v_bottom = (textureRect_.y + textureRect_.w) / texH;
        }

        // アンカーを考慮した座標計算
        float left = -size_.x * anchor_.x;
        float right = size_.x * (1.0f - anchor_.x);
        float top = -size_.y * anchor_.y;
        float bottom = size_.y * (1.0f - anchor_.y);

        // 頂点データの更新
        // 0:左下, 1:左上, 2:右下, 3:右上
        vertexData_[0] = { {left, bottom, 0.0f, 1.0f}, {u_left, v_bottom}, {0,0,-1} };
        vertexData_[1] = { {left, top, 0.0f, 1.0f},    {u_left, v_top},    {0,0,-1} };
        vertexData_[2] = { {right, bottom, 0.0f, 1.0f}, {u_right, v_bottom}, {0,0,-1} };
        vertexData_[3] = { {right, top, 0.0f, 1.0f},    {u_right, v_top},    {0,0,-1} };
    }
}
