#pragma once
#include <unordered_map>
#include <string>

struct PlayerStatusData {
	int maxHp = 3;
	int attackPower = 1;
	int requiredExp = 100;
	float anchorRechargeTime = 3.0f;
};

// プレイヤーステータスのテーブルを管理するクラス
class PlayerStatusRepository
{
public:
	static PlayerStatusRepository* GetInstance();

	// 起動時に読み込み
	void Load(const std::string& filePath);

	// 指定したレベルのステータスを取得
	PlayerStatusData GetStatusByLevel(int level) const;

private:
	PlayerStatusRepository() = default;
	~PlayerStatusRepository() = default;

	// レベルをキーとしたステータスマップ
	std::unordered_map<int, PlayerStatusData> statusTable_;
	// デフォルトのステータス
	PlayerStatusData defaultStatus_ = { 3, 1, 100 };
};

