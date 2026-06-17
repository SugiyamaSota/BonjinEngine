#pragma once
#include "BaseObject.h" 

class Object3D : public BaseObject
{
public:
	// --- コンストラクタ・デストラクタ ---
    Object3D();
    ~Object3D() override;

	// 更新・描画処理関数
    void Update(WorldTransform worldTransform, Camera* camera);
    void Update(const Matrix4x4& worldMatrix, Camera* camera);
    void Draw() override;

    /// <summary>
    /// デバッグ描画処理関数
    /// </summary>
    void DrawImGui(const std::string& label) override;

private:

	// Object3D固有のリソースセットアップ関数
    void SetupResources() override;

    // --- Object3D特有のリソース ---
    // WVPリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
    TransformationMatrix* wvpData_ = nullptr;

    // カメラリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_ = nullptr;
    CameraForGPU* cameraData_ = nullptr;

    // トランスフォーム情報
    WorldTransform transform_;
    Matrix4x4 viewMatrix_;
    Matrix4x4 projectionMatrix_;
    Matrix4x4 viewProjectionMatrix_;
};
