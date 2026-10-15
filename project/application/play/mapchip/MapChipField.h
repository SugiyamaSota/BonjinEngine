#pragma once
#include<vector>
#include<string>

#include "Struct.h"
#include"../logic/Data.h"

enum class MapChipType {
	kBlank, // 空白
	kBlock, // ブロック
	kGoal,  // ゴール
};

struct MapChipData {
	std::vector<std::vector<MapChipType>> data;
};

struct EnemySpawnData {
	std::string type;
	float x;
	float y;
};

struct StageHighlightData {
	std::string name;
	Vector3 startWorldPos = { 0.0f, 0.0f, 0.0f };
	Vector3 panDirection = { 0.0f, 0.0f, 0.0f };
	float displayDuration = 2.0f;
};

class MapChipField {
private:
	static inline const float kBlockWidth = 2.0f;
	static inline const float kBlockHeight = 2.0f;

	uint32_t numBlockVertical_ = 0;
	uint32_t numBlockHorizontal_ = 0;

	MapChipData mapChipData_;
	std::vector<EnemySpawnData> enemySpawns_;
	Vector2 playerSpawn_ = { 2.0f, 1.0f };
	std::vector<StageHighlightData> highlights_;

	bool hasGoal_ = false;
	Vector3 goalPosition_ = { 0.0f, 0.0f, 0.0f };

public:
	/// <summary>
	/// データリセット
	/// </summary>
	void ResetMapChipData();

	/// <summary>
	/// ステージファイルを読み込む（拡張子に応じてJSONまたはCSVを自動判定）
	/// </summary>
	/// <param name="filePath">ステージファイルパス</param>
	void LoadStage(const std::string& filePath);

	/// <summary>
	/// CSVファイルから読み込む
	/// </summary>
	/// <param name="filePath"></param>
	void LoadmapChipCsv(const std::string& filePath);

	/// <summary>
	/// JSONファイルからステージデータ（マップ・敵・ハイライト）を読み込む
	/// </summary>
	/// <param name="filePath"></param>
	/// <returns>読み込み成功可否</returns>
	bool LoadFromJson(const std::string& filePath);

	//セッター

	//ゲッター
	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) const;
	Vector3 GetMapChipPosition(float x, float y) const;
	Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) const;
	uint32_t GetNumBlockVirtical() { return numBlockVertical_; };
	uint32_t GetNumBlockHorizontal() { return numBlockHorizontal_; };
	IndexSet GetMapChipIndexSetByPosition(const Vector3& position) const;
	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex) const;
	IndexSet GetMapChipIndexSetByCenter() const;

	const std::vector<EnemySpawnData>& GetEnemySpawns() const { return enemySpawns_; }
	const std::vector<StageHighlightData>& GetHighlights() const { return highlights_; }
	const Vector2& GetPlayerSpawn() const { return playerSpawn_; }
	bool HasGoal() const { return hasGoal_; }
	const Vector3& GetGoalPosition() const { return goalPosition_; }
};
