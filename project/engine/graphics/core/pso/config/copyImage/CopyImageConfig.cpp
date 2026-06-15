#include "CopyImageConfig.h"

#include "rootSignatureBuilder/RootSignatureBuilder.h"

CopyImageConfig::CopyImageConfig(const wchar_t* psPath) : psPath_(psPath) {}

const wchar_t* CopyImageConfig::GetShaderPath(ShaderStage stage) const {
	if (stage == ShaderStage::kVertex) {
		return L"resources/shader/FullScreen.VS.hlsl";
	} else {
		return psPath_;
	}
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> CopyImageConfig::CreateRootSignature(ID3D12Device* device) {
	// コピーイメージRootSignatureBuilder
	RootSignatureBuilder rootSigBuilder;
	rootSigBuilder.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Root Parameter 0: t0 (Color texture)
	D3D12_DESCRIPTOR_RANGE descriptorRangeT0[1] = {};
	descriptorRangeT0[0].BaseShaderRegister = 0; // t0
	descriptorRangeT0[0].NumDescriptors = 1;
	descriptorRangeT0[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeT0[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[3] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeT0);
	rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRangeT0;

	// Root Parameter 1: t1 (Depth texture)
	D3D12_DESCRIPTOR_RANGE descriptorRangeT1[1] = {};
	descriptorRangeT1[0].BaseShaderRegister = 1; // t1
	descriptorRangeT1[0].NumDescriptors = 1;
	descriptorRangeT1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeT1[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeT1);
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeT1;

	// Root Parameter 2: b0 (Constant Buffer View for Material)
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].Descriptor.ShaderRegister = 0; // b0
	rootParameters[2].Descriptor.RegisterSpace = 0;

	for (const auto& param : rootParameters) {
		rootSigBuilder.AddRootParameter(param);
	}

	// 3. サンプラーの設定 (s0, s1)
	D3D12_STATIC_SAMPLER_DESC staticSamplers[2]{};

	// s0: LINEAR
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0; // register(s0)
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// s1: POINT
	staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[1].ShaderRegister = 1; // register(s1)
	staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	for (const auto& sampler : staticSamplers) {
		rootSigBuilder.AddStaticSampler(sampler);
	}

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