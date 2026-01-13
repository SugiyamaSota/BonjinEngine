#include "TextureManager.h"

TextureManager* TextureManager::instance_ = nullptr;

TextureManager* TextureManager::GetInstance() {
	if (instance_ == nullptr) {
		instance_ = new TextureManager();
		instance_->Initialize();
	}
	return instance_;
}

void TextureManager::DestroyInstance() {
	delete instance_;
	instance_ = nullptr;
}

TextureManager::TextureManager() = default;

TextureManager::~TextureManager() {
	ReleaseIntermediateResources();
}

void TextureManager::Initialize() {
	srvHeapBaseOffset_ = 1;
	nextHandleIndex_ = 0;
}

int TextureManager::LoadTexture(const std::string& filePath) {

	// 既にロード済みであれば、既存のインデックスを返す
	auto it = loadedTextures_.find(filePath);
	if (it != loadedTextures_.end()) {
		return it->second;
	}

	// ここからが新しいテクスチャのロード処理
	DirectX::ScratchImage mipImages = LoadTextureInternal(filePath);
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

	// テクスチャリソースの作成
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = CreateTextureResourceInternal(DirectXCommon::GetInstance()->GetDevice(), metadata);

	// テクスチャデータのアップロード
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = UploadTextureDataInternal(
		textureResource.Get(), mipImages, DirectXCommon::GetInstance()->GetDevice(), DirectXCommon::GetInstance()->GetCommandList());

	// SRV (Shader Resource View) の設定
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCPUDescriptorHandle(DirectXCommon::GetInstance()->GetSRVDescriptorHeap(),
		DirectXCommon::GetInstance()->GetSRVSize(), srvHeapBaseOffset_ + nextHandleIndex_);
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = GetGPUDescriptorHandle(DirectXCommon::GetInstance()->GetSRVDescriptorHeap(),
		DirectXCommon::GetInstance()->GetSRVSize(), srvHeapBaseOffset_ + nextHandleIndex_);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	// SRVヒープにSRVを作成
	DirectXCommon::GetInstance()->GetDevice()->CreateShaderResourceView(textureResource.Get(), &srvDesc, cpuHandle);

	// 現在のnextHandleIndex_を新しいテクスチャのインデックスとして使用
	int newIndex = nextHandleIndex_;

	// ★ここが重要な追加・修正点★
	// ロードしたテクスチャの情報を各vectorに格納する
	textureResources_.push_back(textureResource);
	gpuDescriptorHandles_.push_back(gpuHandle);
	cpuDescriptorHandles_.push_back(cpuHandle); // CPUハンドルも保持するなら
	metadatas_.push_back(metadata);

	// ロード済みマップに登録
	loadedTextures_[filePath] = newIndex;

	// アップロード用中間リソースをリストに追加
	pendingUploadResources_.push_back({ intermediateResource, DirectXCommon::GetInstance()->GetFenceValue() });

	// 次のテクスチャのためのインデックスをインクリメント
	nextHandleIndex_++;

	return newIndex; // ロードしたテクスチャのインデックスを返す
}

void TextureManager::ReleaseIntermediateResources() {
	UINT64 completedFenceValue = DirectXCommon::GetInstance()->GetFence()->GetCompletedValue();

	for (auto it = pendingUploadResources_.begin(); it != pendingUploadResources_.end(); ) {
		if (it->fenceValue <= completedFenceValue) {
			it->resource.Reset();
			it = pendingUploadResources_.erase(it);
		} else {
			++it;
		}
	}
}

// GetGPUHandle の引数を int に変更
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetGPUHandle(int textureIndex) const {
	assert(static_cast<uint32_t>(textureIndex) < gpuDescriptorHandles_.size()); // キャストして比較
	return gpuDescriptorHandles_[textureIndex];
}

// GetMetadata の引数を int に変更
const DirectX::TexMetadata& TextureManager::GetMetadata(int textureIndex) const {
	assert(static_cast<uint32_t>(textureIndex) < metadatas_.size()); // キャストして比較
	return metadatas_[textureIndex];
}

DirectX::ScratchImage TextureManager::LoadTextureInternal(const std::string& filePath) {
	// ... (変更なし) ...
	DirectX::ScratchImage image = {};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	DirectX::ScratchImage mipImages = {};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	return mipImages;
}

Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateTextureResourceInternal(ID3D12Device* device, const DirectX::TexMetadata& metadata) {
	// ... (変更なし) ...
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(resource.GetAddressOf()));
	assert(SUCCEEDED(hr));

	return resource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::UploadTextureDataInternal(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
	// ... (変更なし) ...
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(device, intermediateSize);
	UpdateSubresources(commandList, texture, intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);

	return intermediateResource;
}