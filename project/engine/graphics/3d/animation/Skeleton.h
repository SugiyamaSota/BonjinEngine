#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <wrl/client.h>
#include <d3d12.h>

#include "Struct.h"

struct Animation;

struct Joint {
	QuaternionTransform transform;
	Matrix4x4 localMatrix;
	Matrix4x4 skeletonSpaceMatrix;
	std::string name;
	std::vector<int32_t> children;
	int32_t index;
	std::optional<int32_t> parent;
};

struct Skeleton {
	int32_t root = 0;
	std::map<std::string, int32_t> jointMap;
	std::vector<Joint> joints;
};

struct SkinCluster {
	std::vector<Matrix4x4> inverseBindPoseMatrices;
	Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
	D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
	std::span<VertexInfluence> mappedInfluence;
	Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
	std::span<WellForGPU> mappedPalette;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle;
};

class SkeletonBuilder {
public:
	static Skeleton CreateSkeleton(const Node& rootNode);
	static void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float time);
	static void Update(Skeleton& skeleton);

	// ススキンクラスター関連
	static SkinCluster CreateSkinCluster(
		ID3D12Device* device,
		const Skeleton& skeleton,
		const ModelData& modelData);

	static void Update(
		SkinCluster& skinCluster,
		const Skeleton& skeleton);

private:
	static int32_t CreateJoint(
		const Node& node,
		const std::optional<int32_t>& parent,
		std::vector<Joint>& joints);
};
