#include "ParticleConfig.h"

#include "rootSignatureBuilder/RootSignatureBuilder.h"

const wchar_t* ParticleConfigEx::GetShaderPath(ShaderStage stage) const {
	if (stage == ShaderStage::kVertex) {
		return L"resources/shader/particle/Particle.VS.hlsl";
	} else {
		return L"resources/shader/particle/Particle.PS.hlsl";
	}
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> ParticleConfigEx::CreateRootSignature(ID3D12Device* device) {
	// パーティクル
	RootSignatureBuilder rootSigBuilder;
	rootSigBuilder.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	D3D12_DESCRIPTOR_RANGE instanceDescriptorRange[1] = {};
	instanceDescriptorRange[0].BaseShaderRegister = 0; // t0
	instanceDescriptorRange[0].NumDescriptors = 1;
	instanceDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	instanceDescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE textureDescriptorRange[1] = {};
	textureDescriptorRange[0].BaseShaderRegister = 1; // PSの t0
	textureDescriptorRange[0].NumDescriptors = 1;
	textureDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	textureDescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[4] = {};

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // ★ PIXEL -> ALL に変更
	rootParameters[0].Descriptor.ShaderRegister = 0;

	// Root Parameter 1: Instance Data SRV Table (t0 for VS)
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].DescriptorTable.pDescriptorRanges = instanceDescriptorRange; // ★ インスタンス専用の Range を参照
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(instanceDescriptorRange);

	// Root Parameter 2: Texture SRV Table (t0 for PS)
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = textureDescriptorRange; // ★ テクスチャ用の Range を参照 (t0 for PS)
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(textureDescriptorRange);

	// Root Parameter 3: Light CBV (b1)
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1;

	for (int i = 0; i < _countof(rootParameters); ++i) {
		rootSigBuilder.AddRootParameter(rootParameters[i]);
	}

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
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

std::vector<D3D12_INPUT_ELEMENT_DESC> ParticleConfigEx::GetInputElements() {

	return {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT },
		{ "COLOR",   0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT }
	};
}

void ParticleConfigEx::CustomSetupPSO(
	GraphicsPipelineStateBuilder& psoBuilder,
	D3D12_FILL_MODE fillMode,
	D3D12_CULL_MODE cullMode)
{
	// ⭐ CreatePSOInternal の switch 文にあった Model 用の設定をコピペ
	psoBuilder.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	// デプスステンシル設定（元コードの CreateDepthStencil より）
	D3D12_DEPTH_STENCIL_DESC depthDesc{};
	depthDesc.DepthEnable = true;
	depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoBuilder.SetDepthStencilState(depthDesc);

	// ラスタライザー設定（引数で貰ったモードを適用）
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = fillMode;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	psoBuilder.SetRasterizerState(rasterizerDesc);
}
