#pragma once
#include "BaseObject.h" 

class SkyBox : public BaseObject
{
public:
    // --- コンストラクタ・デストラクタ ---
    SkyBox();
    ~SkyBox() override;

    // 更新・描画処理関数
    void Update(WorldTransform worldTransform, Camera* camera);
    void Draw() override;

    /// <summary>
    /// デバッグ描画処理関数
    /// </summary>
    void DrawImGui(const std::string& label) override;

private:

    // SkyBox固有のリソースセットアップ関数
    void SetupResources() override;

    // --- SkyBox特有のリソース ---
    // WVPリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
    TransformationMatrix* wvpData_ = nullptr;

    // カメラリソース
   

    // トランスフォーム情報
    Matrix4x4 viewMatrix_;
    Matrix4x4 projectionMatrix_;
    Matrix4x4 viewProjectionMatrix_;
};