#include "TextureManager.h"
#include"../srv/SrvManager.h"

bool EndsWith(const std::string& str, const std::string& suffix) {
	if (str.size() < suffix.size()) return false;
	return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

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
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetGPUHandle(int textureIndex) const {
	return SrvManager::GetInstance()->GetGPUHandle(static_cast<uint32_t>(textureIndex));
}

// GetMetadata の引数を int に変更
const DirectX::TexMetadata& TextureManager::GetMetadata(int textureIndex) const {
	assert(static_cast<uint32_t>(textureIndex) < metadatas_.size()); // キャストして比較
	return metadatas_[textureIndex];
}

DirectX::ScratchImage TextureManager::LoadTextureInternal(const std::string& filePath) {
	DirectX::ScratchImage image{};
	std::wstring filePathW = std::wstring(filePath.begin(), filePath.end());
	HRESULT hr;

	// 拡張子で分岐
	if (EndsWith(filePath, ".dds")) {
		// DDSファイルの読み込み
		hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	} else {
		// WIC形式（png, jpgなど）の読み込み
		hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	}

	assert(SUCCEEDED(hr));

	// ミップマップの生成（DDSにミップマップが含まれていない場合などのための共通処理）
	// 資料の2枚目に「ミップマップ生成」がある場合はここに追加
	DirectX::ScratchImage mipChain{};
	if (DirectX::IsCompressed(image.GetMetadata().format)) {
		// 圧縮フォーマットならそのまま
		return image;
	} else {
		hr = DirectX::GenerateMipMaps(
			image.GetImages(), image.GetImageCount(), image.GetMetadata(),
			DirectX::TEX_FILTER_DEFAULT, 0, mipChain);
		if (SUCCEEDED(hr)) {
			return mipChain;
		}
	}

	return image;
}

Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateTextureResourceInternal(ID3D12Device* device, const DirectX::TexMetadata& metadata) {
	
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