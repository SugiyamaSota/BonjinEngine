#include "PlayerStatusRepository.h"

#include<fstream>

#include "../../../externals/nlohmann/json.hpp"

PlayerStatusRepository* PlayerStatusRepository::GetInstance() {
	static PlayerStatusRepository instance;
	return &instance;
}

void PlayerStatusRepository::Load(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		return;
	}

	nlohmann::json jsonData;
	file >> jsonData;

	statusTable_.clear();

	// JSON配列をループ処理
	if (jsonData.contains("status_table") && jsonData["status_table"].is_array()) {
		for (const auto& item : jsonData["status_table"]) {
			int level = item.value("level", 1);
			PlayerStatusData status{};
			status.maxHp = item.value("maxHp", 3);
			status.attackPower = item.value("attackPower", 1);
			status.requiredExp = item.value("requiredExp", 100);

			statusTable_[level] = status;
		}
	}
}

PlayerStatusData PlayerStatusRepository::GetStatusByLevel(int level) const {
	auto it = statusTable_.find(level);
	if (it != statusTable_.end()) {
		return it->second;
	}
	return defaultStatus_;
}