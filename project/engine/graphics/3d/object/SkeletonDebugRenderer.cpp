#include "SkeletonDebugRenderer.h"

#include <algorithm>

#include "Camera.h"
#include "DirectXCommon.h"
#include "Matrix.h"
#include "PSOManager.h"
#include "function/function.h"

namespace {
Vector4 GetJointPosition(const Joint& joint) {
	return {
		joint.skeletonSpaceMatrix.m[3][0],
		joint.skeletonSpaceMatrix.m[3][1],
		joint.skeletonSpaceMatrix.m[3][2],
		1.0f
	};
}
}

SkeletonDebugRenderer::~SkeletonDebugRenderer() {
	if (vertexResource_) {
		vertexResource_->Unmap(0, nullptr);
	}
	if (matrixResource_) {
		matrixResource_->Unmap(0, nullptr);
	}
}

void SkeletonDebugRenderer::Initialize(size_t maxJointCount) {
	common_ = DirectXCommon::GetInstance();
	device_ = common_->GetDevice();
	maxVertexCount_ = std::max<size_t>(2, maxJointCount * 2);

	vertexResource_ = CreateBufferResource(device_, sizeof(LineVertex) * maxVertexCount_);
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(LineVertex) * maxVertexCount_);
	vertexBufferView_.StrideInBytes = sizeof(LineVertex);

	matrixResource_ = CreateBufferResource(device_, sizeof(Matrix4x4));
	matrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&matrixData_));
	*matrixData_ = MakeIdentity4x4();
}

void SkeletonDebugRenderer::Update(
	const Skeleton& skeleton,
	const Matrix4x4& worldMatrix,
	const Camera* camera) {

	vertexCount_ = 0;
	for (const Joint& joint : skeleton.joints) {
		if (!joint.parent.has_value() || vertexCount_ + 2 > maxVertexCount_) {
			continue;
		}

		const Joint& parent = skeleton.joints[*joint.parent];
		vertexData_[vertexCount_++] = { GetJointPosition(parent), color_ };
		vertexData_[vertexCount_++] = { GetJointPosition(joint), color_ };
	}

	*matrixData_ = Multiply(worldMatrix, camera->GetViewProjectionMatrix());
}

void SkeletonDebugRenderer::Draw() {
	if (vertexCount_ == 0) {
		return;
	}

	ID3D12GraphicsCommandList* commandList = common_->GetCommandList();
	PSOManager* psoManager = common_->GetPSO();
	ID3D12PipelineState* pipelineState = psoManager->GetPipelineState(
		device_,
		PrimitiveType::kLine,
		BlendMode::kNone,
		D3D12_FILL_MODE_SOLID,
		D3D12_CULL_MODE_NONE);

	commandList->SetGraphicsRootSignature(psoManager->GetRootSignature(PrimitiveType::kLine));
	commandList->SetPipelineState(pipelineState);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandList->SetGraphicsRootConstantBufferView(0, matrixResource_->GetGPUVirtualAddress());
	commandList->DrawInstanced(vertexCount_, 1, 0, 0);
}
