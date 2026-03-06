#include "TextureManager.h"
#include"../srv/SrvManager.h"

TextureManager* TextureManager::GetInstance() {
	static TextureManager instance;
	return &instance;
}

void TextureManager::Finalize() {

}

TextureManager::TextureManager() = default;

TextureManager::~TextureManager() {
	ReleaseIntermediateResources();
}

void TextureManager::Initialize() {

}

int TextureManager::LoadTexture(const std::string& filePath) {
	auto it = loadedTextures_.find(filePath);
	if (it != loadedTextures_.end()) {
		return it->second;
	}

	DirectX::ScratchImage mipImages = LoadTextureInternal(filePath);
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = CreateTextureResourceInternal(DirectXCommon::GetInstance()->GetDevice(), metadata);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = UploadTextureDataInternal(textureResource.Get(), mipImages, DirectXCommon::GetInstance()->GetDevice(), DirectXCommon::GetInstance()->GetCommandList());

	pendingUploadResources_.push_back({ intermediateResource, DirectXCommon::GetInstance()->GetNextFenceValue() });

	// SrvManagerからインデックスを取得してSRV作成
	uint32_t srvIndex = SrvManager::GetInstance()->Allocate();
	SrvManager::GetInstance()->CreateTextureSrv(srvIndex, textureResource.Get());

	int index = (int)textureResources_.size();
	textureResources_.push_back(textureResource);
	metadatas_.push_back(metadata);

	// filePath に対して srvIndex を紐付け
	loadedTextures_[filePath] = (int)srvIndex;
	return (int)srvIndex;
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
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetGPUHandle(int textureIndex) const{
	return SrvManager::GetInstance()->GetGPUHandle(static_cast<uint32_t>(textureIndex));
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