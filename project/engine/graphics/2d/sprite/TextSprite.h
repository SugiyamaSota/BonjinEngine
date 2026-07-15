#pragma once
#include "BaseSprite.h"

namespace Bonjin {

    class TextSprite : public BaseSprite {
    public:
        TextSprite();
        ~TextSprite() override;

        /// <summary>
        /// テキストスプライトの初期化
        /// </summary>
        void Initialize() override;

        /// <summary>
        /// 描画するテキストの設定とテクスチャの生成
        /// </summary>
        void SetText(const std::wstring& text, int fontSize = 32, COLORREF textColor = RGB(255, 255, 255));

        // 基底クラスの純粋仮想関数を実装
        D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle() const override;

    private:
        // GDIを使用した透過テキストテクスチャの生成とGPUへのコピー
        void CreateTextTexture(const std::wstring& text, int fontSize, COLORREF textColor);

        // テクスチャリソース
        Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_ = nullptr;
        uint32_t srvIndex_ = 0;
        bool hasSrv_ = false;

        uint32_t texWidth_ = 0;
        uint32_t texHeight_ = 0;
    };

}
