#pragma once
#include"Struct.h"

// 衝突情報
struct CollisionMapInfo {
	bool isHotTop_ = false;
	bool isLandin_ = false;
	bool isHitWall_ = false;
	Vector3 movement_;
};

// オブジェクトの４隅
enum Corner {
    kRightBottom,
    kLeftBottom,
    kRightTop,
    kLeftTop,
    kNumCorner
};

// オブジェクトの向き
enum class LRDirection {
	kRight,
	kLeft,
};

//
struct IndexSet {
	uint32_t xIndex;
	uint32_t yIndex;
};

// 矩形
struct Rect {
	float left;
	float right;
	float bottom;
	float top;
};
