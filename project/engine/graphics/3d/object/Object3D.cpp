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

    if (skinCluster_.has_value()) {
        if (skinCluster_->influenceResource) skinCluster_->influenceResource->Unmap(0, nullptr);
        if (skinCluster_->paletteResource) skinCluster_->paletteResource->Unmap(0, nullptr);
    }

    wvpResource_.Reset();
    cameraResource_.Reset();

    if (skinCluster_.has_value()) {
        skinCluster_->influenceResource.Reset();
        skinCluster_->paletteResource.Reset();
    }

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

    if (!modelData_.skinClusterData.empty()) {
        skeleton_ = SkeletonBuilder::CreateSkeleton(modelData_.rootNode);
        skinCluster_ = SkeletonBuilder::CreateSkinCluster(device_, *skeleton_, modelData_);
    } else {
        skeleton_ = std::nullopt;
        skinCluster_ = std::nullopt;
    }
}

void Object3D::Update(WorldTransform worldTransform, Camera* camera) {
    transform_ = worldTransform;

    if (primitiveType_ == PrimitiveType::kSkyBox) {
        transform_.translate = camera->GetWorldPosition();
    }

    Matrix4x4 worldMat = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    Update(worldMat, camera);
}

void Object3D::Update(const Matrix4x4& worldMatrix, Camera* camera) {
    Matrix4x4 worldMat = worldMatrix;

    wvpData_->World = worldMat;
    wvpData_->WVP = Multiply(worldMat, camera->GetViewProjectionMatrix());
    wvpData_->WorldInverseTranspose = Inverse(Transpose(worldMat));

    cameraData_->worldPosition = camera->GetWorldPosition();

    if (skinCluster_.has_value() && skeleton_.has_value()) {
        SkeletonBuilder::Update(*skinCluster_, *skeleton_);
    }
}

void Object3D::Draw() {
    auto commandList = common_->GetCommandList();

    PrimitiveType drawType = primitiveType_;
    if (skinCluster_.has_value() && drawType == PrimitiveType::kObject3D) {
        drawType = PrimitiveType::kSkinningObject3D;
    }

    ID3D12PipelineState* pso = common_->GetPSO()->GetPipelineState(
        device_, drawType, blendMode_, fillMode_, cullMode_
    );

    commandList->SetGraphicsRootSignature(common_->GetPSO()->GetRootSignature(drawType));
    commandList->SetPipelineState(pso);

    // VBVの設定 (スキニング時は複数VBVを設定)
    if (skinCluster_.has_value() && drawType == PrimitiveType::kSkinningObject3D) {
        D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {
            vertexBufferView_,
            skinCluster_->influenceBufferView
        };
        commandList->IASetVertexBuffers(0, 2, vbvs);
    } else {
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    }

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

    if (drawType == PrimitiveType::kObject3D || drawType == PrimitiveType::kSkinningObject3D) {
        commandList->SetGraphicsRootDescriptorTable(7, TextureManager::GetInstance()->GetGPUHandle(envTextureHandle_));
    }

    // スキニング時は MatrixPalette もバインドする
    if (skinCluster_.has_value() && drawType == PrimitiveType::kSkinningObject3D) {
        commandList->SetGraphicsRootDescriptorTable(8, skinCluster_->paletteSrvHandle.second);
    }

    if (!modelData_.indices.empty()) {
        commandList->IASetIndexBuffer(&indexBufferView_);
        commandList->DrawIndexedInstanced(static_cast<UINT>(modelData_.indices.size()), 1, 0, 0, 0);
    } else {
        commandList->DrawInstanced(static_cast<UINT>(modelData_.vertices.size()), 1, 0, 0);
    }
}

void Object3D::DrawImGui(const std::string& label) {
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

        // 💡 ここから Object3D 特有の拡張項目
        if (ImGui::TreeNode("Object3D Specular")) {
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
