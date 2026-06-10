#include "CopyImageConfig.h"

#include "rootSignatureBuilder/RootSignatureBuilder.h"

const wchar_t* CopyImageConfig::GetShaderPath(ShaderStage stage) const {
	if (stage == ShaderStage::kVertex) {
		return L"resources/shader/FullScreen.VS.hlsl";
	} else {
		return L"resources/shader/LuminanceBasedOutline.PS.hlsl";
	}
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> CopyImageConfig::CreateRootSignature(ID3D12Device* device) {
	// コピーイメージRootSignatureBuilder
	RootSignatureBuilder rootSigBuilder;
	rootSigBuilder.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0; // t0
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 2. ルートパラメータの設定 (DescriptorTableとして設定)
	D3D12_ROOT_PARAMETER rootParameters{};
	rootParameters.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters.DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
		
	// 💡Build()を実行するこのスコープ内であれば、このポインタはまだ有効です
	rootParameters.DescriptorTable.pDescriptorRanges = descriptorRange;

	rootSigBuilder.AddRootParameter(rootParameters);

	// 3. サンプラーの設定 (CopyImage.PS.hlsl の register(s0) に対応)
	D3D12_STATIC_SAMPLER_DESC staticSampler{};
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	staticSampler.ShaderRegister = 0; // register(s0)
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	rootSigBuilder.AddStaticSampler(staticSampler);

	return  rootSigBuilder.Build(device);
}

std::vector<D3D12_INPUT_ELEMENT_DESC> CopyImageConfig::GetInputElements() {
	return {};
}

void CopyImageConfig::CustomSetupPSO(
	GraphicsPipelineStateBuilder& psoBuilder,
	D3D12_FILL_MODE fillMode,
	D3D12_CULL_MODE cullMode)
{
	// 全画面パス（CopyImage）用のトポロジー設定
	psoBuilder.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	// ⭐ 深度テスト・深度書き込みを完全に無効化する
	D3D12_DEPTH_STENCIL_DESC depthDesc{};
	depthDesc.DepthEnable = FALSE;
	depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoBuilder.SetDepthStencilState(depthDesc);

	// ⭐ 全画面描画のため、カリングを完全に無効化 (引数のcullModeを無視してNONEにする)
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = fillMode;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	psoBuilder.SetRasterizerState(rasterizerDesc);
}