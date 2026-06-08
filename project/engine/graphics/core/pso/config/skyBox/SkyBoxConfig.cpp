#include "SkyBoxConfig.h"

const wchar_t* SkyBoxConfig::GetShaderPath(ShaderStage stage) const {
	if (stage == ShaderStage::kVertex) {
		return L"resources/shader/SkyBox.VS.hlsl";
	} else {
		return L"resources/shader/SkyBox.PS.hlsl";
	}
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> SkyBoxConfig::CreateRootSignature(ID3D12Device* device) {
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

	return  skyBoxRootSigBuilder.Build(device);
}

std::vector<D3D12_INPUT_ELEMENT_DESC> SkyBoxConfig::GetInputElements() {
	// ⭐ PSOManager::CreateInputLayout から Model 用の設定をここにコピペ
	return {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT }
	};
}

void SkyBoxConfig::CustomSetupPSO(
	GraphicsPipelineStateBuilder& psoBuilder,
	D3D12_FILL_MODE fillMode,
	D3D12_CULL_MODE cullMode)
{
	// ⭐ CreatePSOInternal の switch 文にあった Model 用の設定をコピペ
	psoBuilder.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	// デプスステンシル設定（元コードの CreateDepthStencil より）
	D3D12_DEPTH_STENCIL_DESC depthDesc{};
	depthDesc.DepthEnable = true;
	depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoBuilder.SetDepthStencilState(depthDesc);

	// ラスタライザー設定（引数で貰ったモードを適用）
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = fillMode;
	rasterizerDesc.CullMode = cullMode;
	psoBuilder.SetRasterizerState(rasterizerDesc);
}
