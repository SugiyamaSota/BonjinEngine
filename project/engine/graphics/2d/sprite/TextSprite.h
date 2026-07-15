#pragma once
#include "../bonjin/BonjinEngine.h"
#include <wrl/client.h>
#include <string>
#include <d3d12.h>

namespace Bonjin {

    class TextSprite {
    public:
        TextSprite();
        ~TextSprite();

        /// <summary>
        /// テキストスプライトの初期化
        /// </summary>
        void Initialize();

        /// <summary>
        /// 描画するテキストの設定とテクスチャの生成
        /// </summary>
        /// <param name="text">テキスト文字列 (ワイド文字列)</param>
        /// <param name="fontSize">フォントサイズ</param>
        /// <param name="textColor">文字色</param>
        void SetText(const std::wstring& text, int fontSize = 32, COLORREF textColor = RGB(255, 255, 255));

        /// <summary>
        /// テキストスプライトの更新
        /// </summary>
        void Update();

        /// <summary>
        /// テキストスプライトの描画
        /// </summary>
        void Draw();

        // 各項目のゲッターセッター
        Vector3& Anchor() { return anchor_; }
        Vector2& Size() { return size_; }
        Vector2& Scale() { return scale_; }
        Vector2& Rotate() { return rotate_; }
        Vector2& Translate() { return translate_; }
        Vector4& Color() { return color_; }

    private:
        // GDIを使用した透過テキストテクスチャの生成とGPUへのコピー
        void CreateTextTexture(const std::wstring& text, int fontSize, COLORREF textColor);

        // 頂点リソース
        Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
        VertexData* vertexData_ = nullptr;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

        // インデックスリソース
        Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
        uint32_t* indexData_ = nullptr;
        D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

        // マテリアルリソース
        Material* materialData_ = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;

        // WVP リソース
        TransformationMatrix* wvpData_ = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;

        // テクスチャリソース
        Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_ = nullptr;
        uint32_t srvIndex_ = 0;
        bool hasSrv_ = false;

        // 設定パラメータ
        Vector3 anchor_;
        Vector2 size_;
        Vector2 scale_;
        Vector2 rotate_;
        Vector2 translate_;
        Vector4 color_;

        uint32_t texWidth_ = 0;
        uint32_t texHeight_ = 0;

        // DirectX関連
        DirectXCommon* dxCommon_;
        ID3D12Device* device_;
        ID3D12PipelineState* pso_;

        // 行列
        Matrix4x4 viewMatrix_;
        Matrix4x4 projectionMatrix_;
        Matrix4x4 viewProjectionMatrix_;

        void RefreshVertexData();
    };

}
