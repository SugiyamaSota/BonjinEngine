#include "LineConfig.h"

#include "rootSignatureBuilder/RootSignatureBuilder.h"

const wchar_t* LineConfig::GetShaderPath(ShaderStage stage) const {
	if (stage == ShaderStage::kVertex) {
		return L"resources/shader/Line.VS.hlsl";
	}
	return L"resources/shader/Line.PS.hlsl";
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> LineConfig::CreateRootSignature(ID3D12Device* device) {
	RootSignatureBuilder builder;
	builder.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	D3D12_ROOT_PARAMETER transformation{};
	transformation.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	transformation.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	transformation.Descriptor.ShaderRegister = 0;
	builder.AddRootParameter(transformation);

	return builder.Build(device);
}

std::vector<D3D12_INPUT_ELEMENT_DESC> LineConfig::GetInputElements() {
	return {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT },
	};
}

void LineConfig::CustomSetupPSO(
	GraphicsPipelineStateBuilder& psoBuilder,
	D3D12_FILL_MODE,
	D3D12_CULL_MODE) {

	psoBuilder.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

	D3D12_DEPTH_STENCIL_DESC depthDesc{};
	depthDesc.DepthEnable = true;
	depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoBuilder.SetDepthStencilState(depthDesc);

	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.DepthClipEnable = true;
	psoBuilder.SetRasterizerState(rasterizerDesc);
}
