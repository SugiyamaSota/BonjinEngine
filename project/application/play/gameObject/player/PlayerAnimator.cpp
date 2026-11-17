#include "PlayerAnimator.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace {

const char* const kHipsName = "mixamorig:Hips";
const char* const kDirectory = "resources/models/player";

const Quaternion kIdentity{ 0.0f, 0.0f, 0.0f, 1.0f };

// ハミルトン積。エンジンの行列は「ローカル * 親」なので、クォータニオンでは「親 * ローカル」でワールドになる
Quaternion Multiply(const Quaternion& a, const Quaternion& b) {
	return {
		a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
		a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
		a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
		a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
	};
}

// 単位クォータニオン前提の逆
Quaternion Conjugate(const Quaternion& q) { return { -q.x, -q.y, -q.z, q.w }; }

Quaternion Normalize(const Quaternion& q) {
	float length = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
	if (length <= 0.0f) {
		return kIdentity;
	}
	return { q.x / length, q.y / length, q.z / length, q.w / length };
}

float Dot(const Quaternion& a, const Quaternion& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

Vector3 Add(const Vector3& a, const Vector3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
Vector3 Sub(const Vector3& a, const Vector3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
Vector3 Scale(const Vector3& v, float s) { return { v.x * s, v.y * s, v.z * s }; }
float Length(const Vector3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

Vector3 Rotate(const Quaternion& q, const Vector3& v) {
	Quaternion p{ v.x, v.y, v.z, 0.0f };
	Quaternion r = Multiply(Multiply(q, p), Conjugate(q));
	return { r.x, r.y, r.z };
}

// from方向をto方向へ向ける最短回転
Quaternion FromToRotation(const Vector3& from, const Vector3& to) {
	float fromLength = Length(from);
	float toLength = Length(to);
	if (fromLength < 1e-6f || toLength < 1e-6f) {
		return kIdentity;
	}
	Vector3 f = Scale(from, 1.0f / fromLength);
	Vector3 t = Scale(to, 1.0f / toLength);
	float d = f.x * t.x + f.y * t.y + f.z * t.z;
	if (d < -0.9999f) {
		// 真逆: 適当な垂直軸で180度
		Vector3 axis = std::abs(f.x) < 0.9f ? Vector3{ 0.0f, -f.z, f.y } : Vector3{ f.z, 0.0f, -f.x };
		float l = Length(axis);
		return { axis.x / l, axis.y / l, axis.z / l, 0.0f };
	}
	Vector3 c{ f.y * t.z - f.z * t.y, f.z * t.x - f.x * t.z, f.x * t.y - f.y * t.x };
	return Normalize({ c.x, c.y, c.z, 1.0f + d });
}

// 補間（短い方の経路でnlerp）
Quaternion Blend(const Quaternion& a, const Quaternion& b, float t) {
	float sign = Dot(a, b) < 0.0f ? -1.0f : 1.0f;
	return Normalize({
		a.x + (b.x * sign - a.x) * t,
		a.y + (b.y * sign - a.y) * t,
		a.z + (b.z * sign - a.z) * t,
		a.w + (b.w * sign - a.w) * t,
	});
}

Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
	return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}

// 親の姿勢にローカルを重ねてワールド姿勢を求める
template <typename TPose>
TPose Compose(const TPose& parent, const QuaternionTransform& local) {
	TPose result;
	result.position = Add(parent.position, Rotate(parent.rotate, Scale(local.translate, parent.scale)));
	result.rotate = Normalize(Multiply(parent.rotate, local.rotate));
	result.scale = parent.scale * local.scale.x;
	return result;
}

QuaternionTransform ToTransform(const aiMatrix4x4& matrix) {
	// ModelBuilder::ReadNode と同じ右手系->左手系変換
	aiVector3D scale, translate;
	aiQuaternion rotate;
	matrix.Decompose(scale, rotate, translate);
	QuaternionTransform transform;
	transform.scale = { scale.x, scale.y, scale.z };
	transform.rotate = { rotate.x, -rotate.y, -rotate.z, rotate.w };
	transform.translate = { -translate.x, translate.y, translate.z };
	return transform;
}

bool IsMixamoBone(const std::string& name) { return name.rfind("mixamorig:", 0) == 0; }

// ボーンの向きを決めるための「代表の子」
std::string PrimaryChildName(const std::string& name) {
	static const std::map<std::string, std::string> kOverrides = {
		{ "mixamorig:Hips", "mixamorig:Spine" },
		{ "mixamorig:Spine2", "mixamorig:Neck" },
		{ "mixamorig:LeftHand", "mixamorig:LeftHandMiddle1" },
		{ "mixamorig:RightHand", "mixamorig:RightHandMiddle1" },
	};
	auto it = kOverrides.find(name);
	return it != kOverrides.end() ? it->second : std::string();
}

const char* StateName(PlayerAnimator::State state) {
	switch (state) {
	case PlayerAnimator::State::kIdle: return "Idle";
	case PlayerAnimator::State::kRun: return "Run";
	case PlayerAnimator::State::kJump: return "Jump";
	default: return "-";
	}
}

} // namespace

PlayerAnimator::Clip PlayerAnimator::LoadClip(const std::string& directoryPath, const std::string& filename, bool loop, bool applyRootTranslation) {
	Clip clip;
	clip.loop = loop;
	clip.applyRootTranslation = applyRootTranslation;

	Assimp::Importer importer;
	const std::string fullPath = directoryPath + '/' + filename;
	const aiScene* scene = importer.ReadFile(fullPath, 0);
	if (!scene || scene->mNumAnimations == 0) {
		return clip; // 読めなければ未ロード扱い（レストポーズのまま）
	}

	// ノード階層を親が先に来る順で記録
	struct StackItem {
		const aiNode* node;
		int32_t parent;
	};
	std::vector<StackItem> stack{ { scene->mRootNode, -1 } };
	while (!stack.empty()) {
		StackItem item = stack.back();
		stack.pop_back();
		SourceBone bone;
		bone.name = item.node->mName.C_Str();
		bone.parent = item.parent;
		bone.rest = ToTransform(item.node->mTransformation);
		const int32_t index = static_cast<int32_t>(clip.bones.size());
		clip.boneIndex[bone.name] = index;
		clip.bones.push_back(bone);
		for (uint32_t i = item.node->mNumChildren; i > 0; --i) {
			stack.push_back({ item.node->mChildren[i - 1], index });
		}
	}

	// レストポーズのワールド姿勢
	clip.restGlobal.resize(clip.bones.size());
	for (size_t i = 0; i < clip.bones.size(); ++i) {
		const SourceBone& bone = clip.bones[i];
		GlobalPose parent{};
		if (bone.parent >= 0) {
			parent = clip.restGlobal[bone.parent];
		}
		clip.restGlobal[i] = Compose(parent, bone.rest);
	}

	const aiAnimation* animationAssimp = scene->mAnimations[0];
	float ticksPerSecond = static_cast<float>(animationAssimp->mTicksPerSecond);
	if (ticksPerSecond == 0.0f) {
		ticksPerSecond = 1.0f;
	}
	clip.animation.duration = static_cast<float>(animationAssimp->mDuration) / ticksPerSecond;

	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
		const aiNodeAnim* channel = animationAssimp->mChannels[channelIndex];
		NodeAnimation& nodeAnimation = clip.animation.nodeAnimations[channel->mNodeName.C_Str()];

		for (uint32_t i = 0; i < channel->mNumPositionKeys; ++i) {
			const aiVectorKey& key = channel->mPositionKeys[i];
			nodeAnimation.translate.keyframes.push_back(
				{ static_cast<float>(key.mTime) / ticksPerSecond, { -key.mValue.x, key.mValue.y, key.mValue.z } });
		}
		for (uint32_t i = 0; i < channel->mNumRotationKeys; ++i) {
			const aiQuatKey& key = channel->mRotationKeys[i];
			nodeAnimation.rotate.keyframes.push_back(
				{ static_cast<float>(key.mTime) / ticksPerSecond, { key.mValue.x, -key.mValue.y, -key.mValue.z, key.mValue.w } });
		}
	}

	clip.isLoaded = clip.animation.duration > 0.0f;
	return clip;
}

void PlayerAnimator::BindClip(Clip& clip) const {
	const size_t jointCount = targetNames_.size();
	clip.targetToSource.assign(jointCount, -1);
	clip.align.assign(jointCount, kIdentity);

	for (size_t j = 0; j < jointCount; ++j) {
		if (!IsMixamoBone(targetNames_[j])) {
			continue;
		}
		auto it = clip.boneIndex.find(targetNames_[j]);
		if (it != clip.boneIndex.end()) {
			clip.targetToSource[j] = it->second;
		}
	}

	// レスト時のボーンの向き(自分->代表の子)を元データに揃える回転
	for (size_t j = 0; j < jointCount; ++j) {
		const int32_t source = clip.targetToSource[j];
		if (source < 0) {
			continue;
		}
		std::string childName = PrimaryChildName(targetNames_[j]);
		if (childName.empty()) {
			// 対応が取れる子が1つだけならそれを使う
			int32_t found = 0;
			for (size_t c = 0; c < jointCount; ++c) {
				if (targetParents_[c] == static_cast<int32_t>(j) && clip.targetToSource[c] >= 0) {
					childName = targetNames_[c];
					++found;
				}
			}
			if (found != 1) {
				continue;
			}
		}
		auto targetChild = std::find(targetNames_.begin(), targetNames_.end(), childName);
		auto sourceChild = clip.boneIndex.find(childName);
		if (targetChild == targetNames_.end() || sourceChild == clip.boneIndex.end()) {
			continue;
		}
		const size_t tc = static_cast<size_t>(std::distance(targetNames_.begin(), targetChild));
		const Vector3 targetDirection = Sub(targetRestGlobal_[tc].position, targetRestGlobal_[j].position);
		const Vector3 sourceDirection = Sub(clip.restGlobal[sourceChild->second].position, clip.restGlobal[source].position);
		clip.align[j] = FromToRotation(targetDirection, sourceDirection);
	}

	// 腰の高さの比
	auto targetHips = std::find(targetNames_.begin(), targetNames_.end(), kHipsName);
	auto sourceHips = clip.boneIndex.find(kHipsName);
	clip.heightRatio = 0.0f;
	if (targetHips != targetNames_.end() && sourceHips != clip.boneIndex.end()) {
		const float targetHeight = targetRestGlobal_[std::distance(targetNames_.begin(), targetHips)].position.y;
		const float sourceHeight = clip.restGlobal[sourceHips->second].position.y;
		if (std::abs(sourceHeight) > 1e-6f) {
			clip.heightRatio = targetHeight / sourceHeight;
		}
	}
}

void PlayerAnimator::Initialize(const Skeleton& skeleton) {
	const size_t jointCount = skeleton.joints.size();
	targetNames_.resize(jointCount);
	targetParents_.resize(jointCount);
	targetRestLocal_.resize(jointCount);
	targetRestGlobal_.resize(jointCount);
	for (size_t j = 0; j < jointCount; ++j) {
		const Joint& joint = skeleton.joints[j];
		targetNames_[j] = joint.name;
		targetParents_[j] = joint.parent.has_value() ? *joint.parent : -1;
		targetRestLocal_[j] = joint.transform;
		GlobalPose parent{};
		if (targetParents_[j] >= 0) {
			parent = targetRestGlobal_[targetParents_[j]]; // 親が先に並んでいる
		}
		targetRestGlobal_[j] = Compose(parent, joint.transform);
	}

	clips_[static_cast<size_t>(State::kIdle)] = LoadClip(kDirectory, "Idle.gltf", true, true);
	clips_[static_cast<size_t>(State::kRun)] = LoadClip(kDirectory, "run.gltf", true, true);
	// ジャンプの上下移動は物理側で行うので、Hipsの移動は使わない
	clips_[static_cast<size_t>(State::kJump)] = LoadClip(kDirectory, "Jump.gltf", false, false);
	// Jump.gltfは 0～0.73秒が溜め、0.73秒で踏み切り、1.03秒が頂点、1.33秒で着地。
	// 空中に出てから使うので、踏み切り～着地直前だけ再生して最後のポーズで止める
	clips_[static_cast<size_t>(State::kJump)].startTime = 0.75f;
	clips_[static_cast<size_t>(State::kJump)].endTime = 1.25f;

	for (Clip& clip : clips_) {
		if (clip.isLoaded) {
			BindClip(clip);
		}
	}

	current_ = {};
	previous_ = {};
	blendTimer_ = 0.0f;
}

void PlayerAnimator::AdvanceTime(Playback& playback, float deltaTime) const {
	const Clip& clip = clips_[static_cast<size_t>(playback.state)];
	if (!clip.isLoaded) {
		return;
	}
	const float start = (std::clamp)(clip.startTime, 0.0f, clip.animation.duration);
	const float end = (std::max)(GetEndTime(clip), start);
	playback.time = (std::max)(playback.time, start) + deltaTime * clip.speed;
	if (clip.loop && end > start) {
		playback.time = start + std::fmod(playback.time - start, end - start);
	} else {
		playback.time = (std::min)(playback.time, end);
	}
}

float PlayerAnimator::GetEndTime(const Clip& clip) {
	if (clip.endTime < 0.0f || clip.endTime > clip.animation.duration) {
		return clip.animation.duration;
	}
	return clip.endTime;
}

void PlayerAnimator::Update(float deltaTime, State state) {
	if (state != current_.state) {
		// 状態が変わったらクロスフェードを開始
		previous_ = current_;
		current_ = { state, clips_[static_cast<size_t>(state)].startTime };
		blendTimer_ = blendDuration_;
	}

	AdvanceTime(current_, deltaTime);
	if (blendTimer_ > 0.0f) {
		AdvanceTime(previous_, deltaTime);
		blendTimer_ = (std::max)(blendTimer_ - deltaTime, 0.0f);
	}
}

void PlayerAnimator::SamplePose(const Playback& playback, std::vector<QuaternionTransform>& outLocal) const {
	const size_t jointCount = targetNames_.size();
	outLocal = targetRestLocal_;

	const Clip& clip = clips_[static_cast<size_t>(playback.state)];
	if (!clip.isLoaded || !isEnabled_) {
		return;
	}

	// 1. 元データのワールド姿勢(アニメーション適用後)
	std::vector<GlobalPose> sourceGlobal(clip.bones.size());
	for (size_t i = 0; i < clip.bones.size(); ++i) {
		const SourceBone& bone = clip.bones[i];
		QuaternionTransform local = bone.rest;
		auto animIt = clip.animation.nodeAnimations.find(bone.name);
		if (animIt != clip.animation.nodeAnimations.end()) {
			const NodeAnimation& nodeAnimation = animIt->second;
			if (!nodeAnimation.rotate.keyframes.empty()) {
				local.rotate = AnimationBuilder::CalculateValue(nodeAnimation.rotate.keyframes, playback.time);
			}
			if (!nodeAnimation.translate.keyframes.empty()) {
				local.translate = AnimationBuilder::CalculateValue(nodeAnimation.translate.keyframes, playback.time);
			}
		}
		GlobalPose parent{};
		if (bone.parent >= 0) {
			parent = sourceGlobal[bone.parent];
		}
		sourceGlobal[i] = Compose(parent, local);
	}

	// 2. プレイヤーのワールド回転を決め、ローカルに戻す
	std::vector<Quaternion> targetGlobalRotate(jointCount);
	for (size_t j = 0; j < jointCount; ++j) {
		const int32_t parent = targetParents_[j];
		const Quaternion parentRotate = parent >= 0 ? targetGlobalRotate[parent] : kIdentity;
		const int32_t source = clip.targetToSource[j];

		if (source < 0) {
			targetGlobalRotate[j] = Normalize(Multiply(parentRotate, targetRestLocal_[j].rotate));
			continue;
		}

		// ワールドでの回転量 = アニメ後 * レスト^-1
		const Quaternion delta = Multiply(sourceGlobal[source].rotate, Conjugate(clip.restGlobal[source].rotate));
		Quaternion restRotate = targetRestGlobal_[j].rotate;
		if (useAlign_) {
			restRotate = Multiply(clip.align[j], restRotate);
		}
		targetGlobalRotate[j] = Normalize(Multiply(delta, restRotate));
		outLocal[j].rotate = Normalize(Multiply(Conjugate(parentRotate), targetGlobalRotate[j]));

		// Hipsの移動 (ワールドでの移動量を腰の高さ比でスケールし、親の空間に戻す)
		if (targetNames_[j] == kHipsName && clip.applyRootTranslation) {
			// 上下の揺れだけ使う（前後左右の移動はプレイヤーの移動処理に任せる。In Placeでないデータ対策）
			Vector3 worldDelta = Scale(Sub(sourceGlobal[source].position, clip.restGlobal[source].position), clip.heightRatio);
			worldDelta.x = 0.0f;
			worldDelta.z = 0.0f;
			const float parentScale = parent >= 0 ? targetRestGlobal_[parent].scale : 1.0f;
			const Vector3 localDelta = Scale(Rotate(Conjugate(parentRotate), worldDelta), parentScale > 1e-6f ? 1.0f / parentScale : 1.0f);
			outLocal[j].translate = Add(targetRestLocal_[j].translate, localDelta);
		}
	}
}

void PlayerAnimator::Apply(Skeleton& skeleton) const {
	SamplePose(current_, poseBuffer_);
	if (blendTimer_ > 0.0f && blendDuration_ > 0.0f) {
		SamplePose(previous_, previousPoseBuffer_);
		const float weight = 1.0f - blendTimer_ / blendDuration_;
		for (size_t j = 0; j < poseBuffer_.size(); ++j) {
			poseBuffer_[j].rotate = Blend(previousPoseBuffer_[j].rotate, poseBuffer_[j].rotate, weight);
			poseBuffer_[j].translate = Lerp(previousPoseBuffer_[j].translate, poseBuffer_[j].translate, weight);
		}
	}

	const size_t count = (std::min)(poseBuffer_.size(), skeleton.joints.size());
	for (size_t j = 0; j < count; ++j) {
		skeleton.joints[j].transform = poseBuffer_[j];
	}
	// ジョイントの行列を更新（Object3D::Updateはパレット更新しかしないため）
	SkeletonBuilder::Update(skeleton);
}

void PlayerAnimator::DrawImGui() {
#ifdef USE_IMGUI
	if (ImGui::TreeNode("Animation")) {
		ImGui::Checkbox("Enable", &isEnabled_);
		ImGui::Checkbox("Align Rest Pose (T/A pose)", &useAlign_);
		ImGui::DragFloat("Blend Duration", &blendDuration_, 0.01f, 0.0f, 1.0f);
		ImGui::Text("State: %s  Time: %.2f", StateName(current_.state), current_.time);
		for (size_t i = 0; i < clips_.size(); ++i) {
			Clip& clip = clips_[i];
			ImGui::PushID(static_cast<int>(i));
			ImGui::Text("%s : %s (%.2fs)", StateName(static_cast<State>(i)), clip.isLoaded ? "loaded" : "NOT LOADED", clip.animation.duration);
			ImGui::DragFloat("Speed", &clip.speed, 0.01f, 0.0f, 5.0f);
			// 再生範囲 (Endが-1なら最後まで)
			ImGui::DragFloat("Start Time", &clip.startTime, 0.01f, 0.0f, clip.animation.duration);
			ImGui::DragFloat("End Time", &clip.endTime, 0.01f, -1.0f, clip.animation.duration);
			ImGui::Checkbox("Hips Translation", &clip.applyRootTranslation);
			ImGui::PopID();
		}
		ImGui::TreePop();
	}
#endif
}
