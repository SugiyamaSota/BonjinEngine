#include "Object3D.h"
#include "ModelManager.h"

Object3D::Object3D() : BaseObject() {
    transform_ = InitializeWorldTransform();
    viewMatrix_ = MakeIdentity4x4();
    projectionMatrix_ = MakePerspectiveFovMatrix(0.45f, float(1280) / float(720), 0.1f, 100.0f);
    viewProjectionMatrix_ = MakeIdentity4x4();
}

Object3D::~Object3D() {
    if (wvpResource_) wvpResource_->Unmap(0, nullptr);
    if (cameraResource_) cameraResource_->Unmap(0, nullptr);

    wvpResource_.Reset();
    cameraResource_.Reset();

    wvpData_ = nullptr;
    cameraData_ = nullptr;
}

void Object3D::SetupResources() {
    wvpResource_ = CreateBufferResource(device_, sizeof(TransformationMatrix));
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
    wvpData_->WVP = MakeIdentity4x4();
    wvpData_->World = MakeIdentity4x4();
    wvpData_->WorldInverseTranspose = MakeIdentity4x4();

    cameraResource_ = CreateBufferResource(device_, sizeof(CameraForGPU));
    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));
    cameraData_->worldPosition = Vector3(0.f, 0.f, 0.f);
}

void Object3D::Update(WorldTransform worldTransform, Camera* camera) {
    transform_ = worldTransform;

    if (primitiveType_ == PrimitiveType::kSkyBox) {
        transform_.translate = camera->GetWorldPosition();
    }

    Matrix4x4 worldMat = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    wvpData_->World = worldMat;
    wvpData_->WVP = Multiply(worldMat, camera->GetViewProjectionMatrix());
    wvpData_->WorldInverseTranspose = Inverse(Transpose(worldMat));

    cameraData_->worldPosition = camera->GetWorldPosition();
}

void Object3D::Draw() {
    auto commandList = common_->GetCommandList();

    ID3D12PipelineState* pso = common_->GetPSO()->GetPipelineState(
        device_, primitiveType_, blendMode_, fillMode_, cullMode_
    );

    commandList->SetGraphicsRootSignature(common_->GetPSO()->GetRootSignature(primitiveType_));
    commandList->SetPipelineState(pso);

    // Object3D 特有のバインド
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));
    commandList->SetGraphicsRootConstantBufferView(3, LightManager::GetInstance()->GetDirectionalLightResource()->GetGPUVirtualAddress());
    if (cameraResource_) {
        commandList->SetGraphicsRootConstantBufferView(4, cameraResource_->GetGPUVirtualAddress());
    }
    commandList->SetGraphicsRootConstantBufferView(5, LightManager::GetInstance()->GetPointLightResource()->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(6, LightManager::GetInstance()->GetSpotLightResource()->GetGPUVirtualAddress());

    commandList->DrawInstanced(UINT(modelData_.vertices.size()), 1, 0, 0);
}

void Object3D::DrawImGui() {
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