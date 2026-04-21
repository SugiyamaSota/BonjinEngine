#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <dxcapi.h>
#include<array>
#include <unordered_map>

#include "shaderCompiler/ShaderCompiler.h"
#include "rootSignatureBuilder/RootSignatureBuilder.h"
#include "graphicsPipelineStateBuilder/GraphicsPipelineStateBuilder.h"

enum class PrimitiveType {
	kModel,   // モデル
	kGrid,    // グリッド
	kParticle,
	kSkyBox,
	kCount,
};

enum class ShaderStage {
	kVertex,
	kPixel,
	kCount,
};
//
//enum class FillMode {
//	kSolid,
//	kWireFrame,
//};
//
//enum class CullMode {
//	kNone,
//	kFront,
//	kBack,
//};

class PSOManager {
public:
	PSOManager();
	~PSOManager();

	void Initialize(
		ID3D12Device* device,
		DXGI_FORMAT rtvFormat,
		DXGI_FORMAT dsvFormat);

	// RootSignature
	ID3D12RootSignature* GetRootSignature(PrimitiveType type) const {
		return rootSignatures_[static_cast<size_t>(type)].Get();
	}

	// PipelineStateの取得
	ID3D12PipelineState* GetPipelineState(
		ID3D12Device* device,
		PrimitiveType type,
		BlendMode mode,
		D3D12_FILL_MODE fillMode,
		D3D12_CULL_MODE cullMode);

private:
	/// --- 変数 ---
	// RootSignatureを形状ごとに管理
	std::array<
		Microsoft::WRL::ComPtr<ID3D12RootSignature>,
		static_cast<size_t>(PrimitiveType::kCount)
	> rootSignatures_;

	// pipelineStateをキャッシュするためのマップ
	std::unordered_map<
		size_t,
		Microsoft::WRL::ComPtr<ID3D12PipelineState>
	> psoCache_;

	// pso生成時に必要なフォーマットを保持
	DXGI_FORMAT rtvFormat_ = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT dsvFormat_ = DXGI_FORMAT_UNKNOWN;

	// シェーダーコンパイラー
	ShaderCompiler shaderCompiler_;

	// ⭐ モデルの入力要素ディスクリプタを格納するメンバ配列
	static const size_t kModelInputElements = 3;
	std::array<
		D3D12_INPUT_ELEMENT_DESC,
		kModelInputElements
	> modelInputElementDescs_;

	// ⭐ グリッドの入力要素ディスクリプタを格納するメンバ配列
	static const size_t kGridInputElements = 2;
	std::array<
		D3D12_INPUT_ELEMENT_DESC,
		kGridInputElements
	> gridInputElementDescs_;

	// ⭐ パーティクルの入力要素ディスクリプタを格納するメンバ配列
	static const size_t kParticleInputElements = 3;
	std::array<
		D3D12_INPUT_ELEMENT_DESC,
		kParticleInputElements
	> particleInputElementDescs_;

	// 形状ごとのインプットレイアウトディスク
	std::array<
		D3D12_INPUT_LAYOUT_DESC,
		static_cast<size_t>(PrimitiveType::kCount)>
		inputLayoutDescs_;

	// ラスタライザーディスク
	D3D12_RASTERIZER_DESC rasterizerDesc_{};

	// シェーダー
	std::array<
		std::array<
		Microsoft::WRL::ComPtr<IDxcBlob>,
		static_cast<size_t>(ShaderStage::kCount)
		>,
		static_cast<size_t>(PrimitiveType::kCount)
	> shaderBlobs_;

	// 形状ごとのDepthStencil
	std::array<
		D3D12_DEPTH_STENCIL_DESC,
		static_cast<size_t>(PrimitiveType::kCount)
	> depthStencilDescs_;

	/// --- 関数 ---
	void CreateRootSignature(ID3D12Device* device);
	void CompileAllShaders();
	void CreateInputLayout();
	void CreateDepthStencil();
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSOInternal(
		ID3D12Device* device,
		PrimitiveType type,
		BlendMode mode,
		D3D12_FILL_MODE fillMode,
		D3D12_CULL_MODE cullMode);

	size_t CalculateHash(
		PrimitiveType type,
		BlendMode mode,
		D3D12_FILL_MODE fillMode,
		D3D12_CULL_MODE cullMode)const
	{
		// シンプルなハッシュ生成（より複雑なハッシュ関数を使うと衝突を避けられますが、ここではシンプルに）
		return (size_t)type | ((size_t)mode << 8) | ((size_t)fillMode << 16) | ((size_t)cullMode << 24);
	}

};