#pragma once

#include "Struct.h"

#include <map>
#include <string>
#include <vector>

template <typename tValue>
struct Keyframe {
	float time;
	tValue value;
};

using KeyframeVector3 = Keyframe<Vector3>;
using KeyframeQuaternion = Keyframe<Quaternion>;

template <typename tValue>
struct AnimationCurve {
	std::vector<Keyframe<tValue>> keyframes;
};

struct NodeAnimation {
	AnimationCurve<Vector3> translate;
	AnimationCurve<Quaternion> rotate;
	AnimationCurve<Vector3> scale;
};

struct Animation {
	float duration = 0.0f;
	std::map<std::string, NodeAnimation> nodeAnimations;
};

class AnimationBuilder
{
public:
	static Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename);

	static Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time);

	static Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time);
};
