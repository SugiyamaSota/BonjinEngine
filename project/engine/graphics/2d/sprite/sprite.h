#pragma once
#include "BaseSprite.h"

namespace Bonjin {

    class Sprite : public BaseSprite {
    public:
        Sprite();
        ~Sprite() override;

        /// <summary>
        /// スプライトの初期化
        /// </summary>
        void Initialize(const std::string& textureFilePath);

        // ImGuiの描画
        void DrawImGui();

        // テクスチャの切り取り範囲設定 (ピクセル単位: x, y, width, height)
        void SetTextureRect(float x, float y, float width, float height) { textureRect_ = { x, y, width, height }; }

        // 基底クラスの純粋仮想関数を実装
        D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle() const override;

    protected:
        // Sprite独自の頂点構築処理
        void RefreshVertexData() override;

    private:
        Vector4 textureRect_ = { 0.0f, 0.0f, 0.0f, 0.0f }; // x, y, w, h
        int textureHandle_ = 0;
    };

}