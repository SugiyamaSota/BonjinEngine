#pragma once
#include"../bonjin/BonjinEngine.h"

namespace Bonjin {

    class Sprite {
    public:
        Sprite();
        ~Sprite();

        /// <summary>
        /// スプライトの初期化
        /// </summary>
        void Initialize(const std::string& textureFilePath);

        /// <summary>
        /// スプライトの更新
        /// </summary>
        void Update();

        /// <summary>
        /// スプライトの描画
        /// </summary>
        void Draw();

        // ImGuiの描画
        void DrawImGui();

        // 各項目のゲッターセッター
        Vector3& Anchor() { return anchor_; }
        Vector2& Size() { return size_; }
        Vector2& Scale() { return scale_; }
        Vector2& Rotate() { return rotate_; }
        Vector2& Translate() { return translate_; }
        Vector4& Color() { return color_; }

        // テクスチャの切り取り範囲設定 (ピクセル単位: x, y, width, height)
        void SetTextureRect(float x, float y, float width, float height) { textureRect_ = { x, y, width, height }; }

        void SetFlipX(bool flag) { isFlipX_ = flag; }
        void SetFlipY(bool flag) { isFlipY_ = flag; }

    private:
        // 頂点リソース (1回生成して使い回す)
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

        // 設定パラメータ
        Vector3 anchor_;
        Vector2 size_;
        Vector2 scale_;
        Vector2 rotate_;
        Vector2 translate_;
        Vector4 color_;
        Vector4 textureRect_ = { 0.0f, 0.0f, 0.0f, 0.0f }; // x, y, w, h

        int textureHandle_ = 0;

        // DirectX関連
        DirectXCommon* dxCommon_;
        ID3D12Device* device_;
        ID3D12PipelineState* pso_;

        // 行列
        Matrix4x4 viewMatrix_;
        Matrix4x4 projectionMatrix_;
        Matrix4x4 viewProjectionMatrix_;

        bool isFlipX_ = false;
        bool isFlipY_ = false;

        void UpdateUVTransform();
        void RefreshVertexData(); // リソース再生成を行わず、中身だけ更新する関数
    };

}