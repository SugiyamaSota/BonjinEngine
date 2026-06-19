#pragma once
#include "Struct.h"
#include "Data.h"

class MapChipField;

//AABBとAABB
bool IsCollision(const AABB& aabb1, const AABB& aabb2);

/// <summary>
/// 指定した矩形キャラクターの移動量を、マップにめり込まない値へ補正する
/// </summary>
CollisionMapInfo ResolveMapCollision(
	const MapChipField& mapChipField,
	const Vector3& position,
	const Vector3& movement,
	float width,
	float height);

/// <summary>
/// 指定した矩形キャラクターの足元にブロックがあるか調べる
/// </summary>
bool IsGroundedOnMap(
	const MapChipField& mapChipField,
	const Vector3& position,
	float width,
	float height);
