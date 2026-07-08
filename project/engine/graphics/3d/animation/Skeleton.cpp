#include "Skeleton.h"

#include <cassert>
#include "Animation.h"
#include "Convert.h"
#include "Matrix.h"

Skeleton SkeletonBuilder::CreateSkeleton(const Node& rootNode) {
	Skeleton skeleton;
	skeleton.root = CreateJoint(rootNode, std::nullopt, skeleton.joints);

	for (const Joint& joint : skeleton.joints) {
		skeleton.jointMap.emplace(joint.name, joint.index);
	}

	Update(skeleton);
	return skeleton;
}

void SkeletonBuilder::ApplyAnimation(Skeleton& skeleton, const Animation& animation, float time) {
	for (Joint& joint : skeleton.joints) {
		auto animationIt = animation.nodeAnimations.find(joint.name);
		if (animationIt == animation.nodeAnimations.end()) {
			continue;
		}

		const NodeAnimation& nodeAnimation = animationIt->second;
		if (!nodeAnimation.translate.keyframes.empty()) {
			joint.transform.translate =
				AnimationBuilder::CalculateValue(nodeAnimation.translate.keyframes, time);
		}
		if (!nodeAnimation.rotate.keyframes.empty()) {
			joint.transform.rotate =
				AnimationBuilder::CalculateValue(nodeAnimation.rotate.keyframes, time);
		}
		if (!nodeAnimation.scale.keyframes.empty()) {
			joint.transform.scale =
				AnimationBuilder::CalculateValue(nodeAnimation.scale.keyframes, time);
		}
	}

	Update(skeleton);
}

void SkeletonBuilder::Update(Skeleton& skeleton) {
	for (Joint& joint : skeleton.joints) {
		joint.localMatrix = MakeAffineMatrix(
			joint.transform.scale,
			joint.transform.rotate,
			joint.transform.translate);

		if (joint.parent.has_value()) {
			joint.skeletonSpaceMatrix = Multiply(
				joint.localMatrix,
				skeleton.joints[*joint.parent].skeletonSpaceMatrix);
		} else {
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}

int32_t SkeletonBuilder::CreateJoint(
	const Node& node,
	const std::optional<int32_t>& parent,
	std::vector<Joint>& joints) {

	Joint joint{};
	joint.transform = node.transform;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = MakeIdentity4x4();
	joint.name = node.name;
	joint.index = static_cast<int32_t>(joints.size());
	joint.parent = parent;

	joints.push_back(joint);
	for (const Node& child : node.children) {
		const int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}

	return joint.index;
}

#include "SrvManager.h"
#include "function/function.h"
#include <algorithm>

SkinCluster SkeletonBuilder::CreateSkinCluster(
	ID3D12Device* device,
	const Skeleton& skeleton,
	const ModelData& modelData) {

	SkinCluster skinCluster;

	// 1. palette用のResourceを確保
	skinCluster.paletteResource = CreateBufferResource(device, sizeof(WellForGPU) * skeleton.joints.size());
	WellForGPU* mappedPalette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster.mappedPalette = { mappedPalette, skeleton.joints.size() };

	// SrvManagerからSRVインデックスを確保してSRVを作成
	uint32_t srvIndex = SrvManager::GetInstance()->Allocate();
	skinCluster.paletteSrvHandle.first = SrvManager::GetInstance()->GetCPUHandle(srvIndex);
	skinCluster.paletteSrvHandle.second = SrvManager::GetInstance()->GetGPUHandle(srvIndex);
	SrvManager::GetInstance()->CreateSrv(srvIndex, skinCluster.paletteResource.Get(), SrvType::StructuredBuffer, static_cast<uint32_t>(skeleton.joints.size()), sizeof(WellForGPU));

	// 2. influence用のResourceを確保
	skinCluster.influenceResource = CreateBufferResource(device, sizeof(VertexInfluence) * modelData.vertices.size());
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * modelData.vertices.size());
	skinCluster.mappedInfluence = { mappedInfluence, modelData.vertices.size() };

	// 3. Influence用のVBVを作成
	skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * modelData.vertices.size());
	skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	// 4. InverseBindPoseMatrixの保存領域を作成して、単位行列で埋める
	skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
	std::generate(skinCluster.inverseBindPoseMatrices.begin(), skinCluster.inverseBindPoseMatrices.end(), MakeIdentity4x4);

	// 5. ModelDataのSkinCluster情報を解析してInfluenceの中身を埋める
	for (const auto& jointWeight : modelData.skinClusterData) {
		auto it = skeleton.jointMap.find(jointWeight.first);
		if (it == skeleton.jointMap.end()) {
			continue;
		}
		skinCluster.inverseBindPoseMatrices[it->second] = jointWeight.second.inverseBindPoseMatrix;
		for (const auto& vertexWeight : jointWeight.second.vertexWeights) {
			auto& currentInfluence = skinCluster.mappedInfluence[vertexWeight.vertexIndex];
			for (uint32_t index = 0; index < kNumMaxInfluence; ++index) {
				if (currentInfluence.weights[index] == 0.0f) {
					currentInfluence.weights[index] = vertexWeight.weight;
					currentInfluence.jointIndices[index] = it->second;
					break;
				}
			}
		}
	}

	return skinCluster;
}

void SkeletonBuilder::Update(
	SkinCluster& skinCluster,
	const Skeleton& skeleton) {
	for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex) {
		assert(jointIndex < skinCluster.inverseBindPoseMatrices.size());
		
		skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix =
			Multiply(skinCluster.inverseBindPoseMatrices[jointIndex], skeleton.joints[jointIndex].skeletonSpaceMatrix);
		
		skinCluster.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
			Transpose(Inverse(skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix));
	}
}
