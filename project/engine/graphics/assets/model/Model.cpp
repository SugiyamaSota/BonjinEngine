#include "Model.h"

#include"ModelManager.h"

Model::Model() {
	common = DirectXCommon::GetInstance();
	transform_ = InitializeWorldTransform();
	viewMatrix_ = MakeIdentity4x4();
	projectionMatrix_ = MakePerspectiveFovMatrix(0.45f, float(1280) / float(720), 0.1f, 100.0f);
	viewProjectionMatrix_ = MakeIdentity4x4();
}

void Model::LoadModel(const std::string& fileName) {
	const std::string directoryPath = "resources/models/" + fileName;
	const std::string objFilename = fileName + ".obj";

	// ModelManagerから取得
	modelData_ = &ModelManager::GetInstance()->LoadModel(directoryPath, objFilename);

	// リソースのセットアップ
	SetupResources();

	// テクスチャのロード (ファイルモデル特有)
	textureHandle_ = TextureManager::GetInstance()->LoadTexture(modelData_->material.textureFilepath);
	common->WaitAndResetCommandList();
	TextureManager::GetInstance()->ReleaseIntermediateResources();
}

void Model::CreateSphere(uint32_t subdivision) {
	// ModelBuilderを使用して球体データを生成
	// ※ModelManagerで管理したい場合は ModelManager側に CreateSphereModel を作り、そこからポインタをもらう形が理想的です
	static ModelData sphereData; // インスタンスが破棄されるまでデータを保持
	sphereData = ModelBuilder::CreateSphereModel(subdivision);
	modelData_ = &sphereData;

	// リソースのセットアップ
	SetupResources();

	textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/textures/uvChecker.png");
	common->WaitAndResetCommandList();
	TextureManager::GetInstance()->ReleaseIntermediateResources();
}

void Model::Update(WorldTransform worldTransform, Camera* camera) {
	
	// ワールドトランスフォーム
	transform_ = worldTransform;
	wvpData_->World = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	Matrix4x4 worldViewProjectionMatrix = Multiply(wvpData_->World, camera->GetViewProjectionMatrix());
	wvpData_->WVP = worldViewProjectionMatrix;

	cameraData_->worldPosition = camera->GetWorldPosition();



}

void Model::Draw() {
	// 描画に必要な設定を定義 (モデルの標準設定)
	const D3D12_FILL_MODE currentFillMode = fillMode_;
	const D3D12_CULL_MODE currentCullMode = cullMode_;
	const BlendMode currentBlendMode = blendMode_;

	// PSOを遅延生成/取得するためにデバイスが必要
	ID3D12Device* device = common->GetDevice();

	// PSOManagerの新しいGetPipelineState関数を呼び出し
	ID3D12PipelineState* pso =
		common->GetPSO()->GetPipelineState(
			device,
			PrimitiveType::kModel,
			currentBlendMode,
			currentFillMode,
			currentCullMode
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
	common->GetCommandList()->SetGraphicsRootConstantBufferView(3, LightManager::GetInstance()->GetDirectionalLightResource()->GetGPUVirtualAddress());

	if (cameraResource_) {
		common->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraResource_->GetGPUVirtualAddress());
	}

	// 描画
	common->GetCommandList()->DrawInstanced(UINT(modelData_->vertices.size()), 1, 0, 0);
}

void Model::DrawImGui() {
#ifdef USE_IMGUI
	if (ImGui::BeginTabBar("ModelDebug")) {

		// --- マテリアルタブ ---
		if (ImGui::BeginTabItem("Material")) {
			ImGui::ColorEdit4("Base Color", &materialData_->color.x);
			ImGui::DragFloat("Shininess", &materialData_->shininess, 0.1f, 0.1f, 100.0f);
			ImGui::Checkbox("Enable Lighting", reinterpret_cast<bool*>(&materialData_->enableLighting));
			ImGui::Checkbox("Enable Specular", reinterpret_cast<bool*>(&materialData_->enableSpecular));
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
#endif
}

void Model::SetupResources() {
	// 頂点用のリソース
	vertexResource_ = CreateBufferResource(common->GetDevice(), sizeof(VertexData) * modelData_->vertices.size());
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_->vertices.size());
	vertexBufferView_.StrideInBytes = sizeof(VertexData);
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	std::memcpy(vertexData_, modelData_->vertices.data(), sizeof(VertexData) * modelData_->vertices.size());

	// マテリアル用のリソース
	materialResource_ = CreateBufferResource(common->GetDevice(), sizeof(Material));
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData_->enableLighting = true;
	materialData_->enableSpecular = true;
	materialData_->uvTransform = MakeIdentity4x4();
	materialData_->shininess = 10.f;

	// WVP用のリソース
	wvpResource_ = CreateBufferResource(common->GetDevice(), sizeof(TransformationMatrix));
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
	wvpData_->WVP = MakeIdentity4x4();
	wvpData_->World = MakeIdentity4x4();

	// カメラ
	cameraResource_ = CreateBufferResource(common->GetDevice(), sizeof(CameraForGPU));
	cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));
	cameraData_->worldPosition = Vector3(0.f, 0.f, 0.f);
}

