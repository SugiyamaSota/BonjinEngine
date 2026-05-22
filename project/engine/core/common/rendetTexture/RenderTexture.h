#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <stdint.h>

#include "math/Struct.h"

class RenderTexture
{
public:

	/// コンストラクタ、デストラクタ
	RenderTexture(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
	~RenderTexture() = default;

	/// <summary>
	/// バリア遷移構造体生成
	/// </summary>
	/// <param name="commandList">コマンドリスト</param>
	/// <param name="beforeState">遷移前のリソース状態</param>
	/// <param name="afterState">遷移後のリソース状態</param>
	D3D12_RESOURCE_BARRIER CreateTransitionBarrier(D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	/// <summary>
	/// 画面クリア
	/// </summary>
	/// <param name="commandList">コマンドリスト</param>
	void ClearView(ID3D12GraphicsCommandList* commandList);

	// ゲッター
	ID3D12Resource* GetResource() { return renderTextureResource_.Get(); }
	uint32_t GetSrvIndex() const { return renderTextureSrvIndex_; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle() const { return rtvHandle_; }


private:

	//Vector4 clearColor_ = { 1.0f, 0.0f, 0.0f, 1.0f };
	Vector4 clearColor_ = { 0.1f,0.25f,0.5f,1.0f };
	Microsoft::WRL::ComPtr<ID3D12Resource> renderTextureResource_ = nullptr;
	uint32_t renderTextureSrvIndex_ = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_{};
	D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_COMMON;

private:
	/// 内部用リソース生成関数
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(
		ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor);

};