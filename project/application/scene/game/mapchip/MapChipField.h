#pragma once
#include<vector>
#include<string>

#include "Struct.h"
#include"../logic/Data.h"

enum class MapChipType {
	kBlank, // 空白
	kBlock, // ブロック
	kEnemy, // 敵
	kGoal,  // ゴール
};

struct MapChipData {
	std::vector<std::vector<MapChipType>> data;
};

class MapChipField {
private:
	static inline const float kBlockWidth = 2.0f;
	static inline const float kBlockHeight = 2.0f;

	uint32_t numBlockVertical_ = 0;
	uint32_t numBlockHorizontal_ = 0;

	MapChipData mapChipData_;



public:
	

	/// <summary>
	/// データリセット
	/// </summary>
	void ResetMapChipData();

	/// <summary>
	/// ファイルから読み込む
	/// </summary>
	/// <param name="filePath"></param>
	void LoadmapChipCsv(const std::string& filePath);

	//セッター

	//ゲッター
	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) const;
	Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) const;
	uint32_t GetNumBlockVirtical() { return numBlockVertical_; };
	uint32_t GetNumBlockHorizontal() { return numBlockHorizontal_; };
	IndexSet GetMapChipIndexSetByPosition(const Vector3& position) const;
	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex) const;
	IndexSet GetMapChipIndexSetByCenter() const;
};
