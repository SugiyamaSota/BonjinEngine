#include "SkyBox.h"
#include "ModelManager.h"

SkyBox::SkyBox() : BaseObject() {
    primitiveType_ = PrimitiveType::kSkyBox;
    cullMode_ = D3D12_CULL_MODE_NONE;

    viewMatrix_ = MakeIdentity4x4();
    projectionMatrix_ = MakePerspectiveFovMatrix(0.45f, float(1280) / float(720), 0.1f, 100.0f);
    viewProjectionMatrix_ = MakeIdentity4x4();
}

SkyBox::~SkyBox() {
    if (wvpResource_) wvpResource_->Unmap(0, nullptr);

    wvpResource_.Reset();

    wvpData_ = nullptr;
}

void SkyBox::SetupResources() {
    wvpResource_ = CreateBufferResource(device_, sizeof(TransformationMatrix));
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
    wvpData_->WVP = MakeIdentity4x4();
    wvpData_->World = MakeIdentity4x4();
    wvpData_->WorldInverseTranspose = MakeIdentity4x4();
}

void SkyBox::Update(WorldTransform worldTransform, Camera* camera) {

  Matrix4x4 viewMatrix = camera->GetViewMatrix();
    // ビュー行列の移動成分（4行目の[3][0], [3][1], [3][2]）を 0 に強制上書きしてリセットする
    viewMatrix.m[3][0] = 0.0f;
    viewMatrix.m[3][1] = 0.0f;
    viewMatrix.m[3][2] = 0.0f;

    // スカイボックス自体の大きさを決めるワールド行列（巨大な箱にするために大きなスケールをかける）
    Matrix4x4 worldMatrix = MakeScaleMatrix(Vector3(500.0f, 500.0f, 500.0f)); 

    // 通常通りの合成
    Matrix4x4 worldViewMatrix = Multiply(worldMatrix, viewMatrix);
    Matrix4x4 wvpMatrix = Multiply(worldViewMatrix, camera->GetProjectionMatrix());

    // データを転送
    wvpData_->World = worldMatrix;
    wvpData_->WVP = wvpMatrix;
    wvpData_->WorldInverseTranspose = Inverse(Transpose(worldMatrix));

}

void SkyBox::Draw() {
    auto commandList = common_->GetCommandList();

    ID3D12PipelineState* pso = common_->GetPSO()->GetPipelineState(
        device_, primitiveType_, blendMode_, fillMode_, cullMode_
    );

    commandList->SetGraphicsRootSignature(common_->GetPSO()->GetRootSignature(primitiveType_));
    commandList->SetPipelineState(pso);

    // SkyBox 特有のバインド
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));
    commandList->SetGraphicsRootConstantBufferView(3, LightManager::GetInstance()->GetDirectionalLightResource()->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(5, LightManager::GetInstance()->GetPointLightResource()->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(6, LightManager::GetInstance()->GetSpotLightResource()->GetGPUVirtualAddress());

    if (!modelData_.indices.empty()) {
        commandList->IASetIndexBuffer(&indexBufferView_);
        commandList->DrawIndexedInstanced(static_cast<UINT>(modelData_.indices.size()), 1, 0, 0, 0);
    } else {
        commandList->DrawInstanced(static_cast<UINT>(modelData_.vertices.size()), 1, 0, 0);
    }
}

void SkyBox::DrawImGui(const std::string& label) {
#ifdef USE_IMGUI
    if (!materialData_) return;

    // 一度基底のノードを展開するために、TreeNodeの状態を確認しながら開く形にします。
    if (ImGui::TreeNode(label.c_str())) {
        // 親ノードはすでに開いているので、中身を直接記述していきます

        // 💡 共通項目の描画（二重ノードにならないよう、BaseObject側の関数内 TreeNode と並列にします）
        if (ImGui::TreeNode("Base Material")) {
            ImGui::ColorEdit4("Color", &materialData_->color.x);

            bool enableLight = (materialData_->enableLighting != 0);
            if (ImGui::Checkbox("Enable Lighting", &enableLight)) { materialData_->enableLighting = enableLight ? 1 : 0; }

            ImGui::Separator();

            bool enableEnv = (materialData_->enableEnvironmentMap != 0);
            if (ImGui::Checkbox("Enable EnvMap", &enableEnv)) { materialData_->enableEnvironmentMap = enableEnv ? 1 : 0; }

            if (enableEnv) {
                ImGui::SliderFloat("EnvMap Coefficient", &materialData_->environmentCoefficient, 0.0f, 1.0f);
            }
            ImGui::TreePop();
        }

        // 💡 ここから SkyBox 特有の拡張項目
        if (ImGui::TreeNode("SkyBox Specular")) {
            // 鏡面反射のON/OFF
            bool enableSpecular = (materialData_->enableSpecular != 0);
            if (ImGui::Checkbox("Enable Specular", &enableSpecular)) {
                materialData_->enableSpecular = enableSpecular ? 1 : 0;
            }

            // 鏡面反射の輝度（シャイニネス）
            if (enableSpecular) {
                ImGui::DragFloat("Shininess", &materialData_->shininess, 0.1f, 0.1f, 100.0f);
            }

            ImGui::TreePop();
        }

        // 親ノード（label）を閉じる
        ImGui::TreePop();
    }
#endif
}