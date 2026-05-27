#include "PSOManager.h"
#include <cassert>
#include <format>

// ⭐ シェーダーファイルパスの定数定義 [PrimitiveType][ShaderStage]
static const wchar_t* kShaderPaths[static_cast<size_t>(PrimitiveType::kCount)]
[static_cast<size_t>(ShaderStage::kCount)] = {
	// kModel
	{ L"resources/shader/Object3d.VS.hlsl", L"resources/shader/Object3d.PS.hlsl" },
	// kLine
	{ L"resources/shader/Grid.VS.hlsl", L"resources/shader/Grid.PS.hlsl" },
	// kParticle
	{ L"resources/shader/Particle.VS.hlsl", L"resources/shader/Particle.PS.hlsl" },
	// kSkyBox
   { L"resources/shader/SkyBox.VS.hlsl", L"resources/shader/SkyBox.PS.hlsl" },
   //
	{ L"resources/shader/FullScreen.VS.hlsl", L"resources/shader/GaussianFilter.PS.hlsl" },
};

// ⭐ シェーダープロファイルの定数定義 [ShaderStage]
static const wchar_t* kShaderProfiles[static_cast<size_t>(ShaderStage::kCount)] = {
	L"vs_6_0", // kVertex
	L"ps_6_0"  // kPixel
};

PSOManager::PSOManager() {
	// dxcの初期化
	shaderCompiler_.InitializeDxc();
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

	/// --- DepthStencilState ---
	CreateDepthStencil();

	/// --- Format ---
	rtvFormat_ = rtvFormat;
	dsvFormat_ = dsvFormat;
}

void PSOManager::CreateRootSignature(ID3D12Device* device)
{
	// モデル
	RootSignatureBuilder rootSigBuilder;
	rootSigBuilder.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	D3D12_DESCRIPTOR_RANGE descriptorRangeT0[1] = {};
	descriptorRangeT0[0].BaseShaderRegister = 0;
	descriptorRangeT0[0].NumDescriptors = 1;
	descriptorRangeT0[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeT0[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE descriptorRangeT1[1] = {};
	descriptorRangeT1[0].BaseShaderRegister = 1;
	descriptorRangeT1[0].NumDescriptors = 1;
	descriptorRangeT1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeT1[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[8] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 0;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRangeT0;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeT0);

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // ディレクショナルライト
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1;
	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // カメラ
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[4].Descriptor.ShaderRegister = 2;
	rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // ポイントライト
	rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[5].Descriptor.ShaderRegister = 3;
	rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // スポット
	rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[6].Descriptor.ShaderRegister = 4;


	rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[7].DescriptorTable.pDescriptorRanges = descriptorRangeT1; // ここで T1 を指定
	rootParameters[7].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeT1);


	for (int i = 0; i < _countof(rootParameters); ++i) {
		rootSigBuilder.AddRootParameter(rootParameters[i]);
	}

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	for (int i = 0; i < _countof(staticSamplers); ++i) {
		rootSigBuilder.AddStaticSampler(staticSamplers[i]);
	}

	rootSignatures_[(size_t)PrimitiveType::kModel] = rootSigBuilder.Build(device);

	// グリッド
	RootSignatureBuilder lineRootSigBuilder;
	lineRootSigBuilder.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	D3D12_ROOT_PARAMETER lineRootParameters[1] = {};
	lineRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	lineRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	lineRootParameters[0].Descriptor.ShaderRegister = 0;
	lineRootSigBuilder.AddRootParameter(lineRootParameters[0]);

	rootSignatures_[(size_t)PrimitiveType::kGrid] = lineRootSigBuilder.Build(device);
	// パーティクル
	RootSignatureBuilder particleRootSigBuilder;
	particleRootSigBuilder.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	D3D12_DESCRIPTOR_RANGE particleInstanceDescriptorRange[1] = {};
	particleInstanceDescriptorRange[0].BaseShaderRegister = 0; // t0
	particleInstanceDescriptorRange[0].NumDescriptors = 1;
	particleInstanceDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	particleInstanceDescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE particleTextureDescriptorRange[1] = {};
	particleTextureDescriptorRange[0].BaseShaderRegister = 1; // PSの t0
	particleTextureDescriptorRange[0].NumDescriptors = 1;
	particleTextureDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	particleTextureDescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER particleRootParameters[4] = {};

	particleRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	particleRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // ★ PIXEL -> ALL に変更
	particleRootParameters[0].Descriptor.ShaderRegister = 0;

	// Root Parameter 1: Instance Data SRV Table (t0 for VS)
	particleRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	particleRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	particleRootParameters[1].DescriptorTable.pDescriptorRanges = particleInstanceDescriptorRange; // ★ インスタンス専用の Range を参照
	particleRootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(particleInstanceDescriptorRange);

	// Root Parameter 2: Texture SRV Table (t0 for PS)
	particleRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	particleRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	particleRootParameters[2].DescriptorTable.pDescriptorRanges = particleTextureDescriptorRange; // ★ テクスチャ用の Range を参照 (t0 for PS)
	particleRootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(particleTextureDescriptorRange);

	// Root Parameter 3: Light CBV (b1)
	particleRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	particleRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	particleRootParameters[3].Descriptor.ShaderRegister = 1;

	for (int i = 0; i < _countof(particleRootParameters); ++i) {
		particleRootSigBuilder.AddRootParameter(particleRootParameters[i]);
	}

	D3D12_STATIC_SAMPLER_DESC particleStaticSamplers[1] = {};
	particleStaticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	particleStaticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	particleStaticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	particleStaticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	particleStaticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	particleStaticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	particleStaticSamplers[0].ShaderRegister = 0;
	particleStaticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	for (int i = 0; i < _countof(particleStaticSamplers); ++i) {
		particleRootSigBuilder.AddStaticSampler(particleStaticSamplers[i]);
	}

	rootSignatures_[(size_t)PrimitiveType::kParticle] = particleRootSigBuilder.Build(device);

	// スカイボックス
	RootSignatureBuilder skyBoxRootSigBuilder;
	skyBoxRootSigBuilder.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	D3D12_DESCRIPTOR_RANGE skyBoxDescriptorRange[1] = {};
	skyBoxDescriptorRange[0].BaseShaderRegister = 0;
	skyBoxDescriptorRange[0].NumDescriptors = 2;
	skyBoxDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	skyBoxDescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER skyBoxRootParameters[7] = {};
	skyBoxRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	skyBoxRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	skyBoxRootParameters[0].Descriptor.ShaderRegister = 0;
	skyBoxRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	skyBoxRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	skyBoxRootParameters[1].Descriptor.ShaderRegister = 0;

	skyBoxRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	skyBoxRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	skyBoxRootParameters[2].DescriptorTable.pDescriptorRanges = skyBoxDescriptorRange;
	skyBoxRootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(skyBoxDescriptorRange);

	skyBoxRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // ディレクショナルライト
	skyBoxRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	skyBoxRootParameters[3].Descriptor.ShaderRegister = 1;
	skyBoxRootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // カメラ
	skyBoxRootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	skyBoxRootParameters[4].Descriptor.ShaderRegister = 2;
	skyBoxRootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // ポイントライト
	skyBoxRootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	skyBoxRootParameters[5].Descriptor.ShaderRegister = 3;
	skyBoxRootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // スポット
	skyBoxRootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	skyBoxRootParameters[6].Descriptor.ShaderRegister = 4;


	for (int i = 0; i < _countof(skyBoxRootParameters); ++i) {
		skyBoxRootSigBuilder.AddRootParameter(skyBoxRootParameters[i]);
	}

	D3D12_STATIC_SAMPLER_DESC skyBoxStaticSamplers[1] = {};
	skyBoxStaticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	skyBoxStaticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	skyBoxStaticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	skyBoxStaticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	skyBoxStaticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	skyBoxStaticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	skyBoxStaticSamplers[0].ShaderRegister = 0;
	skyBoxStaticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	for (int i = 0; i < _countof(skyBoxStaticSamplers); ++i) {
		skyBoxRootSigBuilder.AddStaticSampler(skyBoxStaticSamplers[i]);
	}

	rootSignatures_[(size_t)PrimitiveType::kSkyBox] = skyBoxRootSigBuilder.Build(device);

	// コピーイメージRootSignatureBuilder
	RootSignatureBuilder copyImageRootSigBuilder;
	copyImageRootSigBuilder.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// -------------------------------------------------------------------------
	// ★修正：DescriptorRange もビルダーに直接登録するか、Buildまで寿命を維持する
	// -------------------------------------------------------------------------
	// もし Builder クラスに、レンジをまとめて追加できる関数や、
	// パラメータをスマートに登録するヘルパー関数があればそれを使うのがベストです。
	// ここでは、配列の寿命を Build() の瞬間まで維持するために、同じスコープで一気に処理します。

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

			// パスとプロファイルを取得
			const wchar_t* path = kShaderPaths[i][j];
			const wchar_t* profile = kShaderProfiles[j];

			// コンパイルして配列に格納
			shaderBlobs_[i][j] = shaderCompiler_.CompileShader(path, profile);
			assert(shaderBlobs_[i][j] != nullptr && "Shader compilation failed!");
		}
	}
}

void PSOManager::CreateInputLayout()
{
	// モデル
	modelInputElementDescs_[0].SemanticName = "POSITION";
	modelInputElementDescs_[0].SemanticIndex = 0;
	modelInputElementDescs_[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	modelInputElementDescs_[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	modelInputElementDescs_[1].SemanticName = "TEXCOORD";
	modelInputElementDescs_[1].SemanticIndex = 0;
	modelInputElementDescs_[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	modelInputElementDescs_[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	modelInputElementDescs_[2].SemanticName = "NORMAL";
	modelInputElementDescs_[2].SemanticIndex = 0;
	modelInputElementDescs_[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	modelInputElementDescs_[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputLayoutDescs_[(size_t)PrimitiveType::kModel].pInputElementDescs = modelInputElementDescs_.data();
	inputLayoutDescs_[(size_t)PrimitiveType::kModel].NumElements = kModelInputElements;


	// グリッド
	gridInputElementDescs_[0].SemanticName = "POSITION";
	gridInputElementDescs_[0].SemanticIndex = 0;
	gridInputElementDescs_[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	gridInputElementDescs_[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	gridInputElementDescs_[1].SemanticName = "TEXCOORD";
	gridInputElementDescs_[1].SemanticIndex = 0;
	gridInputElementDescs_[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	gridInputElementDescs_[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputLayoutDescs_[(size_t)PrimitiveType::kGrid].pInputElementDescs = gridInputElementDescs_.data();
	inputLayoutDescs_[(size_t)PrimitiveType::kGrid].NumElements = kGridInputElements;

	// パーティクル
	particleInputElementDescs_[0].SemanticName = "POSITION";
	particleInputElementDescs_[0].SemanticIndex = 0;
	particleInputElementDescs_[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	particleInputElementDescs_[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	particleInputElementDescs_[1].SemanticName = "TEXCOORD";
	particleInputElementDescs_[1].SemanticIndex = 0;
	particleInputElementDescs_[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	particleInputElementDescs_[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	particleInputElementDescs_[2].SemanticName = "COLOR";
	particleInputElementDescs_[2].SemanticIndex = 0;
	particleInputElementDescs_[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	particleInputElementDescs_[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputLayoutDescs_[(size_t)PrimitiveType::kParticle].pInputElementDescs = particleInputElementDescs_.data();
	inputLayoutDescs_[(size_t)PrimitiveType::kParticle].NumElements = kParticleInputElements;

	// スカイボックス
	inputLayoutDescs_[(size_t)PrimitiveType::kSkyBox] = inputLayoutDescs_[(size_t)PrimitiveType::kModel];

	// コピーイメージ
	inputLayoutDescs_[(size_t)PrimitiveType::kCopyImage].pInputElementDescs = nullptr;
	inputLayoutDescs_[(size_t)PrimitiveType::kCopyImage].NumElements = 0;
}

void PSOManager::CreateDepthStencil()
{
	// モデル
	depthStencilDescs_[(size_t)PrimitiveType::kModel].DepthEnable = true;
	depthStencilDescs_[(size_t)PrimitiveType::kModel].DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDescs_[(size_t)PrimitiveType::kModel].DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// グリッド
	depthStencilDescs_[(size_t)PrimitiveType::kGrid].DepthEnable = true;
	depthStencilDescs_[(size_t)PrimitiveType::kGrid].DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilDescs_[(size_t)PrimitiveType::kGrid].DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// パーティクル
	depthStencilDescs_[(size_t)PrimitiveType::kParticle].DepthEnable = true;
	depthStencilDescs_[(size_t)PrimitiveType::kParticle].DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDescs_[(size_t)PrimitiveType::kParticle].DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	depthStencilDescs_[(size_t)PrimitiveType::kSkyBox].DepthEnable = true;
	depthStencilDescs_[(size_t)PrimitiveType::kSkyBox].DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilDescs_[(size_t)PrimitiveType::kSkyBox].DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

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
		// ルートシグネチャ、インプットレイアウト、シェーダーを設定
		.SetRootSignature(rootSignatures_[static_cast<size_t>(type)].Get())
		.SetInputLayout(inputLayoutDescs_[static_cast<size_t>(type)])
		.SetVertexShader(shaderBlobs_[static_cast<size_t>(type)][static_cast<size_t>(ShaderStage::kVertex)].Get())
		.SetPixelShader(shaderBlobs_[static_cast<size_t>(type)][static_cast<size_t>(ShaderStage::kPixel)].Get())

		// レンダーターゲットフォーマット、深度ステンシルビューフォーマットを設定
		.SetDepthStencilViewFormat(dsvFormat_)
		.AddRenderTargetFormat(rtvFormat_)

		// ブレンドモードの設定（引数で受け取ったモードを使用）
		.SetBlendMode(mode);

	// 警告防止のため、SampleMaskをデフォルトに明示的に設定
	psoBuilder.SetSampleMask(D3D12_DEFAULT_SAMPLE_MASK);


	// --- 2. ラスタライザーと深度/ステンシルステートのベース設定 ---

	// ラスタライザーはベース設定(rasterizerDesc_)に引数で渡された FillMode/CullMode を適用
	D3D12_RASTERIZER_DESC currentRasterizerDesc = rasterizerDesc_;
	currentRasterizerDesc.FillMode = fillMode;
	currentRasterizerDesc.CullMode = cullMode;
	psoBuilder.SetRasterizerState(currentRasterizerDesc);

	// 深度/ステンシルステートはプリミティブタイプごとの設定を使用
	D3D12_DEPTH_STENCIL_DESC currentDepthStencilDesc = depthStencilDescs_[static_cast<size_t>(type)];
	psoBuilder.SetDepthStencilState(currentDepthStencilDesc);


	// --- 3. プリミティブタイプごとの特殊な設定とオーバーライド ---
	switch (type) {
	case PrimitiveType::kModel:
		// モデルは三角形リスト
		psoBuilder.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		break;

	case PrimitiveType::kParticle:
		// パーティクルは三角形リスト
		psoBuilder.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

		// パーティクル専用のCullModeオーバーライド (両面描画)
		currentRasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		psoBuilder.SetRasterizerState(currentRasterizerDesc);
		break;

	case PrimitiveType::kGrid:
		// グリッドは線リスト
		psoBuilder.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
		// グリッド用の深度/ステンシル設定（深度書き込みなしなど）は、すでに currentDepthStencilDesc で設定済み
		break;
	case PrimitiveType::kSkyBox:
		psoBuilder.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

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