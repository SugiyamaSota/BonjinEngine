#pragma once
#include"ModelBuilder.h"

#include<string>
#include<d3d12.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include<cassert>
#include<random>
#include<array>
#include <functional>

// 挙動を定義する関数の型: 引数に ParticleData の参照を受け取る
struct ParticleData;
using ParticleUpdateFunc = std::function<void(ParticleData&, float deltaTime)>;

struct ParticleData {
	WorldTransform transform;
	float lifeTime;
	float currentTime;
	Vector3 velocity;
	Vector3 acceleration;
	Vector4 color;
	ParticleUpdateFunc updateFunc;
};

class Particle
{
public:
	/// --- 汎用関数 ---
	/// <summary>
	/// コンストラクタ
	//// </summary>
	Particle();

	~Particle();

	/// <summary>
	/// モデルをロード
	/// </summary>
	/// <param name="fileName">モデル名</param>
	void LoadModel(const std::string& fileName);

	/// <summary>
	/// 更新処理
	/// </summary>
	/// <param name="worldTransform">ワールドトランスフォーム</param>
	/// <param name="camera">カメラ</param>
	void Update(Camera* camera);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// パーティクル排出(生成)処理
	/// </summary>
	/// <param name="position">パーティクル発生の中心座標</param>
	/// <param name="range">ランダムな広がり範囲（例：(X, Y, Z)の最大変位）</param>
	/// <param name="numParticles">生成するパーティクル数 (kNumInstance_ 以下)</param>
	/// <param name="minLifetime">最小生存時間（秒）</param>
	/// <param name="maxLifetime">最大生存時間（秒）</param>
	void Emit(const Vector3& position, const Vector3& velocity, const Vector4& color, float lifetime, ParticleUpdateFunc func);

	/// --- 設定関数 ---
	/// <summary>
	/// ライティングの有無
	/// </summary>
	/// <param name="enableLighting">フラグ</param>
	void SetEnableLighting(bool enableLighting) { materialData_->enableLighting = enableLighting; }

	/// <summary>
	/// 色と透明度
	/// </summary>
	/// <param name="color">設定したい色と透明度</param>
	void SetColor(Vector4 color) { materialData_->color = color; }

	/// --- 取得関数 ---
	float GetAlpha() { return materialData_->color.w; }

	void DrawImGui();

private:
	/// --- 変数 ---
	// モデルデータ
	const ModelData* modelData_ = nullptr;

	// 全部で出せる数
	static const uint32_t kNumInstance_ = 1000;
	// 実際に描画するインスタンス数
	uint32_t activeNum_ = 0;

	// それぞれの生存タイマー

	// 乱数生成器
	std::mt19937 randomEngine_;

	float durationTimer_ = 0.0f; // パーティクルを出し続ける全体のタイマー（秒）
	float minLifetime_ = 0.0f;    // 個々の最小生存時間
	float maxLifetime_ = 0.0f;    // 個々の最大生存時間
	Vector3 emitPosition_;        // エミット中心座標
	Vector3 emitRange_;           // エミット範囲

	D3D12_CPU_DESCRIPTOR_HANDLE srvhandleCPU_;
	D3D12_GPU_DESCRIPTOR_HANDLE srvhandleGPU_;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

	// 頂点リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
	VertexData* vertexData_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	// マテリアルリソース
	Material* materialData_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;

	// WVPリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_ = nullptr;
	ParticleForGPU* instancingData_ = nullptr;

	// トランスフォーム
	ParticleData particles_[kNumInstance_];

	// ビュー行列
	Matrix4x4 viewMatrix_;
	// 射影行列
	Matrix4x4 projectionMatrix_;
	// ビュープロジェクション行列
	Matrix4x4 viewProjectionMatrix_;

	// テクスチャハンドル
	int textureHandle_ = 0;

	// dxcommonのインスタンス
	DirectXCommon* common = nullptr;

	bool isSystemActive_ = false;
};