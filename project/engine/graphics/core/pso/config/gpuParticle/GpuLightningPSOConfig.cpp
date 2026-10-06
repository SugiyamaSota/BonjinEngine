#include "GpuLightningPSOConfig.h"
#include "rootSignatureBuilder/RootSignatureBuilder.h"

const wchar_t* GpuLightningPSOConfig::GetShaderPath(ShaderStage stage) const {
    if (stage == ShaderStage::kVertex) {
        return L"resources/shader/gpuParticle/GpuLightning.VS.hlsl";
    } else {
        return L"resources/shader/gpuParticle/GpuLightning.PS.hlsl";
    }
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> GpuLightningPSOConfig::CreateRootSignature(ID3D12Device* device) {
    RootSignatureBuilder rootSigBuilder;
    rootSigBuilder.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // Root Parameter 0: Scene ConstantBuffer (b0) (ViewProjection, BillboardMatrix, CameraPos, Time)
    D3D12_ROOT_PARAMETER rootParamScene{};
    rootParamScene.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParamScene.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParamScene.Descriptor.ShaderRegister = 0;
    rootSigBuilder.AddRootParameter(rootParamScene);

    // Root Parameter 1: StructuredBuffer SRV Table (t0) (ParticleDataGPU)
    D3D12_DESCRIPTOR_RANGE srvRange[1] = {};
    srvRange[0].BaseShaderRegister = 0; // t0
    srvRange[0].NumDescriptors = 1;
    srvRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParamParticles{};
    rootParamParticles.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParamParticles.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParamParticles.DescriptorTable.pDescriptorRanges = srvRange;
    rootParamParticles.DescriptorTable.NumDescriptorRanges = _countof(srvRange);
    rootSigBuilder.AddRootParameter(rootParamParticles);

    // Static Sampler
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

std::vector<D3D12_INPUT_ELEMENT_DESC> GpuLightningPSOConfig::GetInputElements() {
    return {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT }
    };
}

void GpuLightningPSOConfig::CustomSetupPSO(
    GraphicsPipelineStateBuilder& psoBuilder,
    D3D12_FILL_MODE fillMode,
    D3D12_CULL_MODE cullMode)
{
    psoBuilder.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

    D3D12_DEPTH_STENCIL_DESC depthDesc{};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoBuilder.SetDepthStencilState(depthDesc);

    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode = fillMode;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    psoBuilder.SetRasterizerState(rasterizerDesc);
}
