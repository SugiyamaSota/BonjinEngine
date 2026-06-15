#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <stdint.h>

class DepthStencil
{
public:
	
	DepthStencil(ID3D12Device* device);

	// ゲッター
	ID3D12DescriptorHeap* GetDescriptorHeap() const {
		return descriptorHeap_.Get();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() const{
		return descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	}

	ID3D12Resource* GetResource() const {
		return resource_.Get();
	}

	uint32_t GetSrvIndex() const {
		return srvIndex_;
	}

private:
	
	ID3D12Device* device_;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource_ = nullptr;
	D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc_ = {};
	D3D12_DEPTH_STENCIL_DESC desc_ = {};
	uint32_t srvIndex_ = 0;

private:

	/// 内部用リソース生成関数
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(
		ID3D12Device* device, int32_t width, int32_t height);

};