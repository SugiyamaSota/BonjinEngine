#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <stdint.h>

struct Vector4;

class RenderTexture
{
public:

	/// コンストラクタ、デストラクタ
	RenderTexture(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
	~RenderTexture() = default;

	/// <summary>
	/// バリア遷移関数
	/// </summary>
	/// <param name="commandList">コマンドリスト</param>
	/// <param name="beforeState">遷移前のリソース状態</param>
	/// <param name="afterState">遷移後のリソース状態</param>
	void Transition(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	// ゲッター
	ID3D12Resource* GetResource() { return renderTextureResource_.Get(); }
	uint32_t GetSrvIndex() const { return renderTextureSrvIndex_; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle() const { return rtvHandle_; }


private:

	Microsoft::WRL::ComPtr<ID3D12Resource> renderTextureResource_ = nullptr;
	uint32_t renderTextureSrvIndex_ = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_{};
	D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_COMMON;

private:
	/// 内部用リソース生成関数
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(
		ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor);

};