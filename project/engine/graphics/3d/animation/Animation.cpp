#include "Animation.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <cassert>
#include <cmath>

#include "vector.h"

namespace {
Quaternion Slerp(const Quaternion& start, const Quaternion& end, float t) {
	Quaternion result{};
	Quaternion to = end;
	float dot = start.x * to.x + start.y * to.y + start.z * to.z + start.w * to.w;

	if (dot < 0.0f) {
		dot = -dot;
		to.x = -to.x;
		to.y = -to.y;
		to.z = -to.z;
		to.w = -to.w;
	}

	if (dot >= 1.0f - 0.0005f) {
		result.x = start.x + (to.x - start.x) * t;
		result.y = start.y + (to.y - start.y) * t;
		result.z = start.z + (to.z - start.z) * t;
		result.w = start.w + (to.w - start.w) * t;
		float length = std::sqrt(result.x * result.x + result.y * result.y + result.z * result.z + result.w * result.w);
		assert(length != 0.0f);
		result.x /= length;
		result.y /= length;
		result.z /= length;
		result.w /= length;
		return result;
	}

	float theta = std::acos(dot);
	float sinTheta = std::sin(theta);
	float scale0 = std::sin((1.0f - t) * theta) / sinTheta;
	float scale1 = std::sin(t * theta) / sinTheta;

	result.x = scale0 * start.x + scale1 * to.x;
	result.y = scale0 * start.y + scale1 * to.y;
	result.z = scale0 * start.z + scale1 * to.z;
	result.w = scale0 * start.w + scale1 * to.w;
	return result;
}
}

Animation AnimationBuilder::LoadAnimationFile(const std::string& directoryPath, const std::string& filename) {
	Animation animation;
	Assimp::Importer importer;
	std::string fullPath = directoryPath + '/' + filename;
	const aiScene* scene = importer.ReadFile(fullPath.c_str(), 0);
	assert(scene && scene->mNumAnimations != 0);

	aiAnimation* animationAssimp = scene->mAnimations[0];
	float ticksPerSecond = static_cast<float>(animationAssimp->mTicksPerSecond);
	if (ticksPerSecond == 0.0f) {
		ticksPerSecond = 1.0f;
	}
	animation.duration = static_cast<float>(animationAssimp->mDuration) / ticksPerSecond;

	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
		NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex) {
			const aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = static_cast<float>(keyAssimp.mTime) / ticksPerSecond;
			keyframe.value = { -keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
			nodeAnimation.translate.keyframes.push_back(keyframe);
		}

		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex) {
			const aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
			KeyframeQuaternion keyframe;
			keyframe.time = static_cast<float>(keyAssimp.mTime) / ticksPerSecond;
			keyframe.value = { keyAssimp.mValue.x, -keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w };
			nodeAnimation.rotate.keyframes.push_back(keyframe);
		}

		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex) {
			const aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = static_cast<float>(keyAssimp.mTime) / ticksPerSecond;
			keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
			nodeAnimation.scale.keyframes.push_back(keyframe);
		}
	}

	return animation;
}

Vector3 AnimationBuilder::CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time) {
	assert(!keyframes.empty());
	if (keyframes.size() == 1 || time <= keyframes.front().time) {
		return keyframes.front().value;
	}

	for (size_t index = 0; index < keyframes.size() - 1; ++index) {
		size_t nextIndex = index + 1;
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
			float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
			return Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}

	return keyframes.back().value;
}

Quaternion AnimationBuilder::CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time) {
	assert(!keyframes.empty());
	if (keyframes.size() == 1 || time <= keyframes.front().time) {
		return keyframes.front().value;
	}

	for (size_t index = 0; index < keyframes.size() - 1; ++index) {
		size_t nextIndex = index + 1;
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
			float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
			return Slerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}

	return keyframes.back().value;
}
