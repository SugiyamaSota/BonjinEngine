#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "Struct.h"
#include "Convert.h"
#include "DirectXCommon.h"
#include <wrl/client.h>
#include <d3d12.h>
#include <string>

namespace Bonjin {

    class BaseSprite {
    public:
        BaseSprite();
        virtual ~BaseSprite();

        /// <summary>
        /// 共通バッファリソースの初期化
        /// </summary>
        virtual void Initialize();

        /// <summary>
        /// アフィン変換および定数バッファの更新
        /// </summary>
        virtual void Update();

        /// <summary>
        /// スプライトの描画
        /// </summary>
        virtual void Draw();

        // ゲッター (const参照)
        const Vector3& GetAnchor() const { return anchor_; }
        const Vector2& GetSize() const { return size_; }
        const Vector2& GetScale() const { return scale_; }
        const Vector2& GetRotate() const { return rotate_; }
        const Vector2& GetTranslate() const { return translate_; }
        const Vector4& GetColor() const { return color_; }

        // セッター
        void SetAnchor(const Vector3& anchor) { anchor_ = anchor; }
        void SetSize(const Vector2& size) { size_ = size; }
        void SetScale(const Vector2& scale) { scale_ = scale; }
        void SetRotate(const Vector2& rotate) { rotate_ = rotate; }
        void SetTranslate(const Vector2& translate) { translate_ = translate; }
        void SetColor(const Vector4& color) { color_ = color; }

        // テクスチャ範囲と反転設定の共通パラメータ
        void SetFlipX(bool flag) { isFlipX_ = flag; }
        void SetFlipY(bool flag) { isFlipY_ = flag; }

        // 派生クラスごとに固有のGPU記述子ハンドルを返させるための仮想関数
        virtual D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle() const = 0;

    protected:
        // 頂点リソース
        Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
        VertexData* vertexData_ = nullptr;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

        // インデックスリソース
        Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
        uint32_t* indexData_ = nullptr;
        D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

        // マテリアルリソース
        Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
        Material* materialData_ = nullptr;

        // WVPリソース
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
        TransformationMatrix* wvpData_ = nullptr;

        // トランスフォームパラメータ
        Vector3 anchor_;
        Vector2 size_;
        Vector2 scale_;
        Vector2 rotate_;
        Vector2 translate_;
        Vector4 color_;

        bool isFlipX_ = false;
        bool isFlipY_ = false;

        // DirectX関連
        DirectXCommon* dxCommon_;
        ID3D12Device* device_;
        ID3D12PipelineState* pso_;

        // 行列
        Matrix4x4 viewMatrix_;
        Matrix4x4 projectionMatrix_;
        Matrix4x4 viewProjectionMatrix_;

        // 内部メソッド
        virtual void RefreshVertexData();
        virtual void UpdateUVTransform();
    };

}
