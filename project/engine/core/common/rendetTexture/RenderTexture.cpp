#include "RenderTexture.h"
#include <cassert>
#include "windows/WinApp.h"
#include "SrvManager.h"
#include "math/Struct.h"

RenderTexture::RenderTexture(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
	: rtvHandle_(rtvHandle)
{
	const Vector4 kRenderTargetClearValue{ 1.f, 0.f, 0.f, 1.f };

	// 1. リソース生成
	renderTextureResource_ = CreateRenderTextureResource(
		device,
		WinApp::GetInstance()->GetClientWidth(),
		WinApp::GetInstance()->GetClientHeight(),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		kRenderTargetClearValue
	);

	// 2. SRVの割り当てと生成
	SrvManager* srvManager = SrvManager::GetInstance();
	renderTextureSrvIndex_ = srvManager->Allocate();
	srvManager->CreateSrv(renderTextureSrvIndex_, renderTextureResource_.Get(), SrvType::Texture2D);

	// 3. RTVの生成
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	device->CreateRenderTargetView(renderTextureResource_.Get(), &rtvDesc, rtvHandle_);
}

void RenderTexture::Transition(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState) {
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = renderTextureResource_.Get();
	barrier.Transition.StateBefore = beforeState;
	barrier.Transition.StateAfter = afterState;

	commandList->ResourceBarrier(1, &barrier);
}

Microsoft::WRL::ComPtr<ID3D12Resource> RenderTexture::CreateRenderTextureResource(
	ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor)
{
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.Color[0] = clearColor.x;
	clearValue.Color[1] = clearColor.y;
	clearValue.Color[2] = clearColor.z;
	clearValue.Color[3] = clearColor.w;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&clearValue,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}