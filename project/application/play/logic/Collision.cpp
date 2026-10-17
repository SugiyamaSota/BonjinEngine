#include "Collision.h"
#include "../mapchip/MapChipField.h"
#include "vector.h"

#include <array>

namespace {
constexpr float kCollisionInset = 0.01f;

Vector3 GetCornerPosition(const Vector3& center, float width, float height, Corner corner) {
	const Vector3 offsets[kNumCorner] = {
		{+width / 2.0f, -height / 2.0f, 0.0f},
		{-width / 2.0f, -height / 2.0f, 0.0f},
		{+width / 2.0f, +height / 2.0f, 0.0f},
		{-width / 2.0f, +height / 2.0f, 0.0f},
	};
	return Add(center, offsets[static_cast<uint32_t>(corner)]);
}

bool IsBlock(const MapChipField& mapChipField, const Vector3& position) {
	const IndexSet index = mapChipField.GetMapChipIndexSetByPosition(position);
	return mapChipField.GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipType::kBlock;
}
}

bool IsCollision(const AABB& aabb1, const AABB& aabb2) {
	if ((aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) &&
		(aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) &&
		(aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z)) {
		return true;
	} else {
		return false;
	}
}

CollisionMapInfo ResolveMapCollision(
	const MapChipField& mapChipField,
	const Vector3& position,
	const Vector3& movement,
	float width,
	float height) {
	CollisionMapInfo info{};
	info.movement_ = movement;

	// 横方向を先に解決し、その結果を使って縦方向を調べる。
	if (info.movement_.x != 0.0f) {
		const Vector3 nextPosition = Add(position, Vector3{info.movement_.x, 0.0f, 0.0f});
		const Corner topCorner = info.movement_.x > 0.0f ? kRightTop : kLeftTop;
		const Corner bottomCorner = info.movement_.x > 0.0f ? kRightBottom : kLeftBottom;
		// 床や天井との境界を横壁として拾わないよう、上下端から少し内側を調べる。
		const Vector3 top = Add(
			GetCornerPosition(nextPosition, width, height, topCorner),
			Vector3{0.0f, -kCollisionInset, 0.0f});
		const Vector3 bottom = Add(
			GetCornerPosition(nextPosition, width, height, bottomCorner),
			Vector3{0.0f, kCollisionInset, 0.0f});

		if (IsBlock(mapChipField, top) || IsBlock(mapChipField, bottom)) {
			const Vector3 hitPosition = IsBlock(mapChipField, bottom) ? bottom : top;
			const IndexSet index = mapChipField.GetMapChipIndexSetByPosition(hitPosition);
			const Rect rect = mapChipField.GetRectByIndex(index.xIndex, index.yIndex);
			bool resolved = false;

			if (info.movement_.x > 0.0f) {
				const float correction = rect.right - (position.x + width / 2.0f);
				// 既にわずかにめり込んでいる場合は負の補正で押し戻す。
				if (correction < info.movement_.x) {
					info.movement_.x = correction;
					resolved = true;
				}
			} else {
				const float correction = rect.left - (position.x - width / 2.0f);
				// 既にわずかにめり込んでいる場合は正の補正で押し戻す。
				if (correction > info.movement_.x) {
					info.movement_.x = correction;
					resolved = true;
				}
			}
			info.isHitWall_ = resolved;
		}
	}

	if (info.movement_.y != 0.0f) {
		const Vector3 nextPosition = Add(position, info.movement_);
		const Corner leftCorner = info.movement_.y > 0.0f ? kLeftTop : kLeftBottom;
		const Corner rightCorner = info.movement_.y > 0.0f ? kRightTop : kRightBottom;
		// 壁との境界を床や天井として拾わないよう、左右端から少し内側を調べる。
		const Vector3 left = Add(
			GetCornerPosition(nextPosition, width, height, leftCorner),
			Vector3{kCollisionInset, 0.0f, 0.0f});
		const Vector3 right = Add(
			GetCornerPosition(nextPosition, width, height, rightCorner),
			Vector3{-kCollisionInset, 0.0f, 0.0f});

		if (IsBlock(mapChipField, left) || IsBlock(mapChipField, right)) {
			const Vector3 hitPosition = IsBlock(mapChipField, left) ? left : right;
			const IndexSet index = mapChipField.GetMapChipIndexSetByPosition(hitPosition);
			const Rect rect = mapChipField.GetRectByIndex(index.xIndex, index.yIndex);

			if (info.movement_.y > 0.0f) {
				const float correction = rect.bottom - (position.y + height / 2.0f);
				// 浮動小数点誤差などで天井へ少しめり込んでも、下へ押し戻す。
				if (correction < info.movement_.y) {
					info.movement_.y = correction;
					info.isHotTop_ = true;
				}
			} else {
				const float correction = rect.top - (position.y - height / 2.0f);
				// 床へ少しめり込んだ状態も上へ押し戻す。
				if (correction > info.movement_.y) {
					info.movement_.y = correction;
					info.isLandin_ = true;
				}
			}
		}
	}

	return info;
}

bool IsGroundedOnMap(
	const MapChipField& mapChipField,
	const Vector3& position,
	float width,
	float height) {
	constexpr float kGroundProbeDistance = 0.01f;
	const Vector3 leftBottom = Add(
		GetCornerPosition(position, width, height, kLeftBottom),
		Vector3{0.0f, -kGroundProbeDistance, 0.0f});
	const Vector3 rightBottom = Add(
		GetCornerPosition(position, width, height, kRightBottom),
		Vector3{0.0f, -kGroundProbeDistance, 0.0f});
	return IsBlock(mapChipField, leftBottom) || IsBlock(mapChipField, rightBottom);
}
