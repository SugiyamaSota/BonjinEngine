#include "PSOManager.h"
#include <cassert>

// 各形状のconfigクラスのインクルード
#include "config/Object3D/Object3DConfig.h"
#include "config/particle/ParticleConfig.h"
#include "config/skyBox/SkyBoxConfig.h"
#include "config/copyImage/CopyImageConfig.h"



PSOManager::PSOManager() {
	// dxcの初期化
	shaderCompiler_.InitializeDxc();

	// pso生成時にconfigを取得
	configs_[static_cast<size_t>(PrimitiveType::kModel)] = std::make_unique<Object3DConfig>();      // object3D
	configs_[static_cast<size_t>(PrimitiveType::kParticle)] = std::make_unique<ParticleConfigEx>(); // particle
	configs_[static_cast<size_t>(PrimitiveType::kSkyBox)] = std::make_unique<SkyBoxConfig>();       // skyBox

	// 各種ポストエフェクトのConfig
	configs_[static_cast<size_t>(PrimitiveType::kPostEffectFullScreen)] = std::make_unique<CopyImageConfig>(L"resources/shader/FullScreen.PS.hlsl");
	configs_[static_cast<size_t>(PrimitiveType::kPostEffectBoxFilter)] = std::make_unique<CopyImageConfig>(L"resources/shader/BoxFilter.PS.hlsl");
	configs_[static_cast<size_t>(PrimitiveType::kPostEffectGaussianFilter)] = std::make_unique<CopyImageConfig>(L"resources/shader/GaussianFilter.PS.hlsl");
	configs_[static_cast<size_t>(PrimitiveType::kPostEffectLuminanceOutline)] = std::make_unique<CopyImageConfig>(L"resources/shader/LuminanceBasedOutline.PS.hlsl");
	configs_[static_cast<size_t>(PrimitiveType::kPostEffectDepthOutline)] = std::make_unique<CopyImageConfig>(L"resources/shader/DepthBasedOutline.PS.hlsl");
	configs_[static_cast<size_t>(PrimitiveType::kPostEffectRadialBlur)] = std::make_unique<CopyImageConfig>(L"resources/shader/RadialBlur.PS.hlsl");
	configs_[static_cast<size_t>(PrimitiveType::kPostEffectDissolve)] = std::make_unique<CopyImageConfig>(L"resources/shader/Dissolve.PS.hlsl");
	configs_[static_cast<size_t>(PrimitiveType::kPostEffectRandomNoise)] = std::make_unique<CopyImageConfig>(L"resources/shader/RandomNoise.PS.hlsl");

}

PSOManager::~PSOManager() {}

void PSOManager::Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat)
{
	
	/// --- 形状ごとのRoorSignature ---
	CreateRootSignature(device);

	/// --- 共通の初期化 ---
	// RasterizerState
	rasterizerDesc_.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc_.FillMode = D3D12_FILL_MODE_SOLID;

	/// --- シェーダー ---
	CompileAllShaders();

	/// --- InputLayout ---
	CreateInputLayout();

	/// --- Format ---
	rtvFormat_ = rtvFormat;
	dsvFormat_ = dsvFormat;
}

void PSOManager::CreateRootSignature(ID3D12Device* device)
{

	for (size_t idx = 0; idx < static_cast<size_t>(PrimitiveType::kCount); ++idx) {
		if (configs_[idx]) {
			rootSignatures_[idx] = configs_[idx]->CreateRootSignature(device);
		}
	}


}

void PSOManager::CompileAllShaders() {
	for (int i = 0; i < static_cast<int>(PrimitiveType::kCount); ++i) {
		// 全てがConfig管理されていれば、未登録はただのバグなのでアサートで落とす
		assert(configs_[i] != nullptr && "Config must be initialized for all primitive types!");

		for (int j = 0; j < static_cast<int>(ShaderStage::kCount); ++j) {
			ShaderStage stage = static_cast<ShaderStage>(j);
			const wchar_t* path = configs_[i]->GetShaderPath(stage);

			// もし「頂点シェーダーはあるが、ピクセルシェーダーはない」といったケースがある場合
			if (path == nullptr || wcslen(path) == 0) {
				continue;
			}

			const wchar_t* profile = (stage == ShaderStage::kVertex) ? L"vs_6_0" : L"ps_6_0";
			shaderBlobs_[i][j] = shaderCompiler_.CompileShader(path, profile);
			assert(shaderBlobs_[i][j] != nullptr && "Shader compilation failed!");
		}
	}
}

void PSOManager::CreateInputLayout()
{

	// 形状ごとのInputElementDescをconfigから取得してキャッシュ
	for (size_t idx = 0; idx < static_cast<size_t>(PrimitiveType::kCount); ++idx) {
		if (configs_[idx]) {
			inputElementsCache_[idx] =
				configs_[idx]->GetInputElements();
		}
	}

	// 形状ごとのInputLayoutDescを構築
	for (int idx = 0; idx < static_cast<int>(PrimitiveType::kCount); ++idx) {
		if(configs_[idx]) {
			inputLayoutDescs_[idx].pInputElementDescs = inputElementsCache_[idx].data();
			inputLayoutDescs_[idx].NumElements = static_cast<UINT>(inputElementsCache_[idx].size());
		}
		
	}
}

ID3D12PipelineState* PSOManager::GetPipelineState(
	ID3D12Device* device,
	PrimitiveType type,
	BlendMode mode,
	D3D12_FILL_MODE fillMode,
	D3D12_CULL_MODE cullMode)
{

	// キーの生成
	size_t key = CalculateHash(type, mode, fillMode, cullMode);

	// キャッシュを検索
	if (psoCache_.count(key)) {
		return psoCache_.at(key).Get();
	}

	// ヒットしなかったら作成
	Microsoft::WRL::ComPtr<ID3D12PipelineState>newPso =
		CreatePSOInternal(device, type, mode, fillMode, cullMode);

	// キャッシュに保存
	psoCache_[key] = newPso;

	// 作成したpsoを返す
	return newPso.Get();

}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PSOManager::CreatePSOInternal(
	ID3D12Device* device,
	PrimitiveType type,
	BlendMode mode,
	D3D12_FILL_MODE fillMode,
	D3D12_CULL_MODE cullMode)
{
	GraphicsPipelineStateBuilder psoBuilder;

	// --- 1. 共通の設定 (Builderを使用) ---
	psoBuilder
		.SetRootSignature(rootSignatures_[static_cast<size_t>(type)].Get())
		.SetInputLayout(inputLayoutDescs_[static_cast<size_t>(type)])
		.SetVertexShader(shaderBlobs_[static_cast<size_t>(type)][static_cast<size_t>(ShaderStage::kVertex)].Get())
		.SetPixelShader(shaderBlobs_[static_cast<size_t>(type)][static_cast<size_t>(ShaderStage::kPixel)].Get())
		.SetDepthStencilViewFormat(dsvFormat_)
		.AddRenderTargetFormat(rtvFormat_)
		.SetBlendMode(mode)
		.SetSampleMask(D3D12_DEFAULT_SAMPLE_MASK);


	// --- 2. ラスタライザーと深度/ステンシルステートのベース設定 ---
	D3D12_RASTERIZER_DESC currentRasterizerDesc = rasterizerDesc_;
	currentRasterizerDesc.FillMode = fillMode;
	currentRasterizerDesc.CullMode = cullMode;
	psoBuilder.SetRasterizerState(currentRasterizerDesc);

	D3D12_DEPTH_STENCIL_DESC currentDepthStencilDesc = depthStencilDescs_[static_cast<size_t>(type)];
	psoBuilder.SetDepthStencilState(currentDepthStencilDesc);

	// --- 3. プリミティブタイプごとの特殊な設定とオーバーライド ---
	size_t idx = static_cast<size_t>(type);
	if (configs_[idx]) {
		configs_[idx]->CustomSetupPSO(psoBuilder, fillMode, cullMode);
		return psoBuilder.Build(device);
	}

	return psoBuilder.Build(device);
}
