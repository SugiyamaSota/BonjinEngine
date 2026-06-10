#include "PSOManager.h"
#include <cassert>

#include "rootSignatureBuilder/RootSignatureBuilder.h"

#include "config/Object3D/Object3DConfig.h"
#include "config/particle/ParticleConfig.h"
#include "config/skyBox/SkyBoxConfig.h"

// ⭐ シェーダーファイルパスの定数定義 [PrimitiveType][ShaderStage]
static const wchar_t* kShaderPaths[static_cast<size_t>(PrimitiveType::kCount)]
[static_cast<size_t>(ShaderStage::kCount)] = {
	// kModel
	{ L"resources/shader/Object3d.VS.hlsl", L"resources/shader/Object3d.PS.hlsl" },
	// kParticle
	{ L"resources/shader/Particle.VS.hlsl", L"resources/shader/Particle.PS.hlsl" },
	// kSkyBox
   { L"resources/shader/SkyBox.VS.hlsl", L"resources/shader/SkyBox.PS.hlsl" },
   //
	{ L"resources/shader/FullScreen.VS.hlsl", L"resources/shader/FullScreen.PS.hlsl" },
};

// ⭐ シェーダープロファイルの定数定義 [ShaderStage]
static const wchar_t* kShaderProfiles[static_cast<size_t>(ShaderStage::kCount)] = {
	L"vs_6_0", // kVertex
	L"ps_6_0"  // kPixel
};

PSOManager::PSOManager() {
	// dxcの初期化
	shaderCompiler_.InitializeDxc();

	// pso生成時にconfigを取得
	configs_[static_cast<size_t>(PrimitiveType::kModel)] = std::make_unique<Object3DConfig>(); // object3D
	configs_[static_cast<size_t>(PrimitiveType::kParticle)] = std::make_unique<ParticleConfigEx>();   // particle
	configs_[static_cast<size_t>(PrimitiveType::kSkyBox)] = std::make_unique<SkyBoxConfig>();  // skyBox

}

PSOManager::~PSOManager() {}

void PSOManager::Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat)
{
	for (size_t idx = 0; idx < static_cast<size_t>(PrimitiveType::kCount); ++idx) {
		if (configs_[idx]) {
			rootSignatures_[idx] = configs_[idx]->CreateRootSignature(device);
		}
	}

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

	/// --- DepthStencilState ---
	CreateDepthStencil();

	/// --- Format ---
	rtvFormat_ = rtvFormat;
	dsvFormat_ = dsvFormat;
}

void PSOManager::CreateRootSignature(ID3D12Device* device)
{
	// コピーイメージRootSignatureBuilder
	RootSignatureBuilder copyImageRootSigBuilder;
	copyImageRootSigBuilder.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	D3D12_DESCRIPTOR_RANGE copyImageDescriptorRange[1] = {};
	copyImageDescriptorRange[0].BaseShaderRegister = 0; // t0
	copyImageDescriptorRange[0].NumDescriptors = 1;
	copyImageDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	copyImageDescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 2. ルートパラメータの設定 (DescriptorTableとして設定)
	D3D12_ROOT_PARAMETER copyImageRootParameters{};
	copyImageRootParameters.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	copyImageRootParameters.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	copyImageRootParameters.DescriptorTable.NumDescriptorRanges = _countof(copyImageDescriptorRange);
	// 💡Build()を実行するこのスコープ内であれば、このポインタはまだ有効です
	copyImageRootParameters.DescriptorTable.pDescriptorRanges = copyImageDescriptorRange;

	copyImageRootSigBuilder.AddRootParameter(copyImageRootParameters);

	// 3. サンプラーの設定 (CopyImage.PS.hlsl の register(s0) に対応)
	D3D12_STATIC_SAMPLER_DESC copyImageStaticSampler{};
	copyImageStaticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	copyImageStaticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	copyImageStaticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	copyImageStaticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	copyImageStaticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	copyImageStaticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	copyImageStaticSampler.ShaderRegister = 0; // register(s0)
	copyImageStaticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	copyImageRootSigBuilder.AddStaticSampler(copyImageStaticSampler);

	// -------------------------------------------------------------------------
	// 💡途中に挟まっていた手動の「D3D12_ROOT_SIGNATURE_DESC」のローカル宣言は
	// Builderのビルド処理と衝突・混乱する原因になるので丸ごと削除してOKです！
	// -------------------------------------------------------------------------

	// すべての変数が生きているこの瞬間に、Builderにまとめてシグネチャを作らせる
	rootSignatures_[(size_t)PrimitiveType::kCopyImage] = copyImageRootSigBuilder.Build(device);

}

void PSOManager::CompileAllShaders() {
	for (int i = 0; i < static_cast<int>(PrimitiveType::kCount); ++i) {
		for (int j = 0; j < static_cast<int>(ShaderStage::kCount); ++j) {

			const wchar_t* path = nullptr;

			if (configs_[i]) {
				path = configs_[i]->GetShaderPath(static_cast<ShaderStage>(j));
			} else {

				path = kShaderPaths[i][j];
			}

			const wchar_t* profile = kShaderProfiles[j];
			shaderBlobs_[i][j] = shaderCompiler_.CompileShader(path, profile);
			assert(shaderBlobs_[i][j] != nullptr && "Shader compilation failed!");
		}
	}
}

void PSOManager::CreateInputLayout()
{


	for (size_t idx = 0; idx < static_cast<size_t>(PrimitiveType::kCount); ++idx) {
		if (configs_[idx]) {
			inputElementsCache_[idx] =
				configs_[idx]->GetInputElements();
		}
	}

	for (int idx = 0; idx < static_cast<int>(PrimitiveType::kCount); ++idx) {
		if(configs_[idx]) {
			inputLayoutDescs_[idx].pInputElementDescs = inputElementsCache_[idx].data();
			inputLayoutDescs_[idx].NumElements = static_cast<UINT>(inputElementsCache_[idx].size());
		}
		
	}

	// コピーイメージ
	inputLayoutDescs_[(size_t)PrimitiveType::kCopyImage].pInputElementDescs = nullptr;
	inputLayoutDescs_[(size_t)PrimitiveType::kCopyImage].NumElements = 0;
}

void PSOManager::CreateDepthStencil()
{
	depthStencilDescs_[(size_t)PrimitiveType::kCopyImage].DepthEnable = false;
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

	switch (type) {
	case PrimitiveType::kParticle:
		break;
	case PrimitiveType::kCopyImage:
		psoBuilder.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

		// 2. カリングを完全に無効化 (裏表に関わらず絶対に描画する)
		currentRasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		psoBuilder.SetRasterizerState(currentRasterizerDesc);

		// 3. 深度テスト・深度書き込みを完全に無効化する
		currentDepthStencilDesc.DepthEnable = FALSE;
		currentDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		psoBuilder.SetDepthStencilState(currentDepthStencilDesc);
		break;
	case PrimitiveType::kCount:
		assert(false && "Invalid PrimitiveType used for PSO creation.");
		return nullptr;
	}


	return psoBuilder.Build(device);
}