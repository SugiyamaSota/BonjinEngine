#include "SkinningObject3DConfig.h"

#include "rootSignatureBuilder/RootSignatureBuilder.h"

const wchar_t* SkinningObject3DConfig::GetShaderPath(ShaderStage stage) const {
	if (stage == ShaderStage::kVertex) {
		return L"resources/shader/object3d/SkinningObject3d.VS.hlsl";
	} else {
		return L"resources/shader/object3d/Object3d.PS.hlsl";
	}
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> SkinningObject3DConfig::CreateRootSignature(ID3D12Device* device) {
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

	// MatrixPalette (StructuredBuffer) 用
	D3D12_DESCRIPTOR_RANGE descriptorRangeT2[1] = {};
	descriptorRangeT2[0].BaseShaderRegister = 0; // VS の t0
	descriptorRangeT2[0].NumDescriptors = 1;
	descriptorRangeT2[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeT2[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[9] = {};
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
	rootParameters[7].DescriptorTable.pDescriptorRanges = descriptorRangeT1;
	rootParameters[7].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeT1);

	rootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[8].DescriptorTable.pDescriptorRanges = descriptorRangeT2;
	rootParameters[8].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeT2);

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

	return  rootSigBuilder.Build(device);
}

std::vector<D3D12_INPUT_ELEMENT_DESC> SkinningObject3DConfig::GetInputElements() {
	return {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT },
		{ "WEIGHT",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT }, // Slot 1
		{ "INDEX",    0, DXGI_FORMAT_R32G32B32A32_SINT,  1, D3D12_APPEND_ALIGNED_ELEMENT }  // Slot 1
	};
}

void SkinningObject3DConfig::CustomSetupPSO(
	GraphicsPipelineStateBuilder& psoBuilder,
	D3D12_FILL_MODE fillMode,
	D3D12_CULL_MODE cullMode)
{
	psoBuilder.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	D3D12_DEPTH_STENCIL_DESC depthDesc{};
	depthDesc.DepthEnable = true;
	depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoBuilder.SetDepthStencilState(depthDesc);

	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = fillMode;
	rasterizerDesc.CullMode = cullMode;
	psoBuilder.SetRasterizerState(rasterizerDesc);
}
