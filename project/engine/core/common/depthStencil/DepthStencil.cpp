#include "DepthStencil.h"

#include <cassert>

#include "WinApp.h"
#include "function/function.h"
#include "SrvManager.h"

DepthStencil::DepthStencil(ID3D12Device* device) : device_(device) 
{
	
	//DepthStencilTextureをウィンドウのサイズで作成
	resource_ = CreateDepthStencilTextureResource(device, WinApp::GetInstance()->GetClientWidth(), WinApp::GetInstance()->GetClientHeight());
	//DSV
	descriptorHeap_ = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	//DSVの設定
	viewDesc_.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	viewDesc_.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	//DSVHeapの先頭にDSVをつくる
	device_->CreateDepthStencilView(resource_.Get(), &viewDesc_, descriptorHeap_->GetCPUDescriptorHandleForHeapStart());

	//DepthStencilStateの設定
	//Depthの機能を有効化
	desc_.DepthEnable = true;
	//書き込みします
	desc_.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//比較関数
	desc_.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// SRVの割り当てと生成
	SrvManager* srvManager = SrvManager::GetInstance();
	srvIndex_ = srvManager->Allocate();
	srvManager->CreateSrv(srvIndex_, resource_.Get(), SrvType::DepthTexture);

}

Microsoft::WRL::ComPtr<ID3D12Resource>  DepthStencil::CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height) {
	//生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	//利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	//深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(resource.GetAddressOf()));
	assert(SUCCEEDED(hr));

	return resource;
}
