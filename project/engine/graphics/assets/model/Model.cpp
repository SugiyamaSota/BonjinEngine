#include "Model.h"

Model::Model() {
	common = DirectXCommon::GetInstance();
	transform_ = InitializeWorldTransform();
	viewMatrix_ = MakeIdentity4x4();
	projectionMatrix_ = MakePerspectiveFovMatrix(0.45f, float(1280) / float(720), 0.1f, 100.0f);
	viewProjectionMatrix_ = MakeIdentity4x4();
}

void Model::LoadModel(const std::string& fileName) {
	// モデルファイル読み込み
	modelData_ = ModelBuilder::LoadObjFile("resources/models/" + fileName, fileName + ".obj");

	// 頂点用のリソース
	vertexResource_ = CreateBufferResource(common->GetDevice(), sizeof(VertexData) * modelData_.vertices.size());
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices.size());
	vertexBufferView_.StrideInBytes = sizeof(VertexData);
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	std::memcpy(vertexData_, modelData_.vertices.data(), sizeof(VertexData) * modelData_.vertices.size());

	// マテリアル用のリソース
	materialResource_ = CreateBufferResource(common->GetDevice(), sizeof(Material));
	materialData_ = nullptr;
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData_->enableLighting = false;
	materialData_->uvTransform = MakeIdentity4x4();

	// WVP用のリソース
	wvpResource_ = CreateBufferResource(common->GetDevice(), sizeof(TransformationMatrix));
	wvpData_ = nullptr;
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
	wvpData_->WVP = MakeIdentity4x4();
	wvpData_->World = MakeIdentity4x4();

	// テクスチャ
	textureHandle_ = TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilepath);
	common->WaitAndResetCommandList();
	TextureManager::GetInstance()->ReleaseIntermediateResources();
}

void Model::Update(WorldTransform worldTransform, Camera* camera) {
	// ワールドトランスフォーム
	transform_ = worldTransform;
	wvpData_->World = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	Matrix4x4 worldViewProjectionMatrix = Multiply(wvpData_->World, camera->GetViewProjectionMatrix());
	wvpData_->WVP = worldViewProjectionMatrix;
}

void Model::Draw() {
	// 描画に必要な設定を定義 (モデルの標準設定)
	const D3D12_FILL_MODE defaultFillMode = D3D12_FILL_MODE_SOLID;
	const D3D12_CULL_MODE defaultCullMode = D3D12_CULL_MODE_BACK;
	const BlendMode currentBlendMode = BlendMode::kNormal; // 既存のブレンドモード

	// PSOを遅延生成/取得するためにデバイスが必要
	ID3D12Device* device = common->GetDevice(); // 💡 commonがDeviceを提供すると仮定

	// PSOManagerの新しいGetPipelineState関数を呼び出し
	ID3D12PipelineState* pso =
		common->GetPSO()->GetPipelineState(
			device,
			PrimitiveType::kModel,
			currentBlendMode,
			defaultFillMode,
			defaultCullMode
		);

	// PSOの設定
	common->GetCommandList()->SetGraphicsRootSignature(common->GetPSO()->GetRootSignature(PrimitiveType::kModel));
	common->GetCommandList()->SetPipelineState(pso);

	//　モデルの描画
	// VBV
	common->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
	// 形状を設定
	common->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// マテリアルCBufferの場所を設定
	common->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
	// wvp用のCBufferの場所を設定
	common->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	// SRV用のdescriptionTavleの先頭を設定
	common->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));
	// 光
	common->GetCommandList()->SetGraphicsRootConstantBufferView(3, common->GetDirectionalLightResource()->GetGPUVirtualAddress());
	// 描画
	common->GetCommandList()->DrawInstanced(UINT(modelData_.vertices.size()), 1, 0, 0);
}

