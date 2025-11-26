#pragma once
#include"../bonjin/BonjinEngine.h"
#include"../color/Color.h"

class Sprite {
public:
    Sprite();
    ~Sprite(); // リソースを解放するためのデストラクタ

    /// <summary>
    /// スプライトの初期化
    /// </summary>
    /// <param name="worldTransform">ワールド変換</param>
    /// <param name="textureFilePath">テクスチャファイルのパス</param>
    void Initialize(WorldTransform transform, Color color, Vector3 anchor, Vector2 size, const std::string& textureFilePath);

    /// <summary>
    /// スプライトの更新
    /// </summary>
    /// <param name="worldTransform">ワールド変換</param>
    /// <param name="color">色</param>
    void Update(WorldTransform transform, Color color);

    /// <summary>
    /// スプライトの描画
    /// </summary>
    void Draw();

    void DrawImGui();

    void  SetColor(Vector4 color) { materialData_->color = color; }

    void SetFlipX(bool flag) { isFlipX_ = flag; }
    void SetFlipY(bool flag) { isFlipY_ = flag; }

private:
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

    WorldTransform transform_;
    int textureHandle_ = 0;

    // ビュー行列
    Matrix4x4 viewMatrix_;
    // 射影行列
    Matrix4x4 projectionMatrix_;
    // ビュープロジェクション行列
    Matrix4x4 viewProjectionMatrix_;

    bool isFlipX_ = false;
    bool isFlipY_ = false;

    void UpdateUVTransform();
};