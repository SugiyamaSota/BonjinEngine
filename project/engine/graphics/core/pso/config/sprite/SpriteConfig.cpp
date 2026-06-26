#include "SpriteConfig.h"

#include "rootSignatureBuilder/RootSignatureBuilder.h"

const wchar_t* SpriteConfig::GetShaderPath(ShaderStage stage) const {
	if (stage == ShaderStage::kVertex) {
		return L"resources/shader/Sprite.VS.hlsl";
	}
	return L"resources/shader/Sprite.PS.hlsl";
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> SpriteConfig::CreateRootSignature(ID3D12Device* device) {
	RootSignatureBuilder rootSigBuilder;
	rootSigBuilder.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	D3D12_DESCRIPTOR_RANGE descriptorRangeT0[1] = {};
	descriptorRangeT0[0].BaseShaderRegister = 0;
	descriptorRangeT0[0].NumDescriptors = 1;
	descriptorRangeT0[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeT0[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[3] = {};
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

	for (const auto& param : rootParameters) {
		rootSigBuilder.AddRootParameter(param);
	}

	D3D12_STATIC_SAMPLER_DESC staticSampler{};
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	staticSampler.ShaderRegister = 0;
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootSigBuilder.AddStaticSampler(staticSampler);

	return rootSigBuilder.Build(device);
}

std::vector<D3D12_INPUT_ELEMENT_DESC> SpriteConfig::GetInputElements() {
	return {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT }
	};
}

void SpriteConfig::CustomSetupPSO(
	GraphicsPipelineStateBuilder& psoBuilder,
	D3D12_FILL_MODE fillMode,
	D3D12_CULL_MODE cullMode)
{
	psoBuilder.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	D3D12_DEPTH_STENCIL_DESC depthDesc{};
	depthDesc.DepthEnable = FALSE;
	depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoBuilder.SetDepthStencilState(depthDesc);

	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = fillMode;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	psoBuilder.SetRasterizerState(rasterizerDesc);
}
