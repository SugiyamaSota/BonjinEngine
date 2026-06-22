#include "Skeleton.h"

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
