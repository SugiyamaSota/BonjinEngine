#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

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

class SkeletonBuilder {
public:
	static Skeleton CreateSkeleton(const Node& rootNode);
	static void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float time);
	static void Update(Skeleton& skeleton);

private:
	static int32_t CreateJoint(
		const Node& node,
		const std::optional<int32_t>& parent,
		std::vector<Joint>& joints);
};
