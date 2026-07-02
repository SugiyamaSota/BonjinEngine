#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <dxcapi.h>
#include<array>
#include <unordered_map>
#include <memory>

#include "shaderCompiler/ShaderCompiler.h"
#include "graphicsPipelineStateBuilder/GraphicsPipelineStateBuilder.h"
#include "config/IPipelineConfig.h"

enum class PrimitiveType {
	kObject3D,   // 3Dオブジェクト
	kSprite,
	kParticle,
	kSkyBox,
	kLine,
	kPostEffectFullScreen,
	kPostEffectBoxFilter,
	kPostEffectGaussianFilter,
	kPostEffectLuminanceOutline,
	kPostEffectDepthOutline,
	kPostEffectRadialBlur,
	kPostEffectDissolve,
	kPostEffectRandomNoise,
	kSkinningObject3D,
	kCount,
};

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

	// 各形状ごとのconfigを保持
	std::array<
		std::unique_ptr<IPipelineConfig>,
		static_cast<size_t>(PrimitiveType::kCount)
	> configs_;

	// inputElementsを生存させるため保持
	std::array<
		std::vector<D3D12_INPUT_ELEMENT_DESC>,
		static_cast<size_t>(PrimitiveType::kCount)
	> inputElementsCache_;

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

	// ⭐ パーティクルの入力要素ディスクリプタを格納するメンバ配列
	static const size_t kParticleInputElements = 3;
	std::array<
		D3D12_INPUT_ELEMENT_DESC,
		kParticleInputElements
	> particleInputElementDescs_{};

	static const size_t kCopyImageInputElements = 0;
	std::array<
		D3D12_INPUT_ELEMENT_DESC,
		kCopyImageInputElements
	> copyImageInputElementDescs_{};

	// 形状ごとのインプットレイアウトディスク
	std::array<
		D3D12_INPUT_LAYOUT_DESC,
		static_cast<size_t>(PrimitiveType::kCount)>
		inputLayoutDescs_{};

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
	> depthStencilDescs_{};

	/// --- 関数 ---
	void CreateRootSignature(ID3D12Device* device);
	void CompileAllShaders();
	void CreateInputLayout();
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
