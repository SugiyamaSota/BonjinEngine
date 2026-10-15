#include "MapChipField.h"

#include <map>
#include <fstream>
#include <sstream>
#include <cassert>
#include <algorithm> // std::max用

#include "../../../../externals/nlohmann/json.hpp"

namespace {
    std::map<std::string, MapChipType> mapChipTable = {
        {"0", MapChipType::kBlank},
        {"1", MapChipType::kBlock},
        {"2", MapChipType::kGoal},
        {"G", MapChipType::kGoal},
        {"g", MapChipType::kGoal},
    };
}

void MapChipField::ResetMapChipData() {
    mapChipData_.data.clear();
    enemySpawns_.clear();
    highlights_.clear();
    playerSpawn_ = { 2.0f, 1.0f };
    hasGoal_ = false;
    goalPosition_ = { 0.0f, 0.0f, 0.0f };
    numBlockVertical_ = 0;
    numBlockHorizontal_ = 0;
}

void MapChipField::LoadStage(const std::string& filePath) {
    // 拡張子判定
    if (filePath.length() >= 5 && 
        (filePath.compare(filePath.length() - 5, 5, ".json") == 0 ||
         filePath.compare(filePath.length() - 5, 5, ".JSON") == 0)) {
        if (!LoadFromJson(filePath)) {
            // JSON読み込み失敗時はアサートまたはログ
            assert(false && "Failed to load stage json");
        }
    } else {
        LoadmapChipCsv(filePath);
    }
}

bool MapChipField::LoadFromJson(const std::string& filePath) {
    ResetMapChipData();

    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    nlohmann::json root;
    try {
        file >> root;
    } catch (const std::exception&) {
        file.close();
        return false;
    }
    file.close();

    // 1. プレイヤースポーン位置
    if (root.contains("player_spawn") && root["player_spawn"].is_object()) {
        playerSpawn_.x = root["player_spawn"].value("x", playerSpawn_.x);
        playerSpawn_.y = root["player_spawn"].value("y", playerSpawn_.y);
    }

    // 2. マップ構造
    if (root.contains("map") && root["map"].is_array()) {
        const auto& mapArray = root["map"];
        for (uint32_t rowIndex = 0; rowIndex < mapArray.size(); ++rowIndex) {
            std::vector<MapChipType> mapChipLine;

            if (mapArray[rowIndex].is_string()) {
                // 文字列配列の場合（例: "1000000001"）
                const std::string lineStr = mapArray[rowIndex].get<std::string>();
                for (uint32_t colIndex = 0; colIndex < lineStr.length(); ++colIndex) {
                    char c = lineStr[colIndex];
                    if (c == '1') {
                        mapChipLine.push_back(MapChipType::kBlock);
                    } else if (c == '2' || c == 'G' || c == 'g') {
                        mapChipLine.push_back(MapChipType::kGoal);
                        hasGoal_ = true;
                    } else if (c == 'P' || c == 'p') {
                        mapChipLine.push_back(MapChipType::kBlank);
                        playerSpawn_ = { static_cast<float>(colIndex), static_cast<float>(rowIndex) };
                    } else {
                        mapChipLine.push_back(MapChipType::kBlank);
                    }
                }
            } else if (mapArray[rowIndex].is_array()) {
                // 数値配列の場合（例: [1, 0, 0, 1]）
                for (uint32_t colIndex = 0; colIndex < mapArray[rowIndex].size(); ++colIndex) {
                    int val = mapArray[rowIndex][colIndex].get<int>();
                    if (val == 1) {
                        mapChipLine.push_back(MapChipType::kBlock);
                    } else if (val == 2) {
                        mapChipLine.push_back(MapChipType::kGoal);
                        hasGoal_ = true;
                    } else {
                        mapChipLine.push_back(MapChipType::kBlank);
                    }
                }
            }

            numBlockHorizontal_ = std::max(numBlockHorizontal_, static_cast<uint32_t>(mapChipLine.size()));
            mapChipData_.data.push_back(mapChipLine);
        }
    }

    numBlockVertical_ = static_cast<uint32_t>(mapChipData_.data.size());

    // 各行の長さを最大幅に揃える
    for (auto& line : mapChipData_.data) {
        if (line.size() < numBlockHorizontal_) {
            line.resize(numBlockHorizontal_, MapChipType::kBlank);
        }
    }

    // ゴール座標の計算
    if (hasGoal_) {
        for (uint32_t y = 0; y < numBlockVertical_; ++y) {
            for (uint32_t x = 0; x < numBlockHorizontal_; ++x) {
                if (mapChipData_.data[y][x] == MapChipType::kGoal) {
                    goalPosition_ = GetMapChipPositionByIndex(x, y);
                    break;
                }
            }
        }
    }

    // 3. 敵の配置
    if (root.contains("enemies") && root["enemies"].is_array()) {
        for (const auto& item : root["enemies"]) {
            std::string type = item.value("type", "normalenemy");
            float x = item.value("x", 0.0f);
            float y = item.value("y", 0.0f);
            enemySpawns_.push_back({ type, x, y });
        }
    }

    // 4. ハイライト演出データ
    if (root.contains("highlights") && root["highlights"].is_array()) {
        for (const auto& item : root["highlights"]) {
            StageHighlightData hl;
            hl.name = item.value("name", "Highlight");
            std::string type = item.value("type", "index");
            hl.displayDuration = item.value("duration", item.value("displayDuration", 2.0f));

            if (item.contains("panDirection") && item["panDirection"].is_object()) {
                hl.panDirection.x = item["panDirection"].value("x", 0.0f);
                hl.panDirection.y = item["panDirection"].value("y", 0.0f);
                hl.panDirection.z = item["panDirection"].value("z", 0.0f);
            } else {
                hl.panDirection.x = item.value("panDirectionX", 0.0f);
                hl.panDirection.y = item.value("panDirectionY", 0.0f);
                hl.panDirection.z = item.value("panDirectionZ", 0.0f);
            }

            if (type == "goal") {
                if (hasGoal_) {
                    hl.startWorldPos = goalPosition_;
                } else {
                    hl.startWorldPos = GetMapChipPosition(static_cast<float>(numBlockHorizontal_ - 3), 1.0f);
                }
            } else if (type == "enemy") {
                size_t enemyIdx = item.value("enemyIndex", 0);
                if (enemyIdx < enemySpawns_.size()) {
                    hl.startWorldPos = GetMapChipPosition(enemySpawns_[enemyIdx].x, enemySpawns_[enemyIdx].y);
                } else if (!enemySpawns_.empty()) {
                    hl.startWorldPos = GetMapChipPosition(enemySpawns_[0].x, enemySpawns_[0].y);
                } else {
                    hl.startWorldPos = GetMapChipPosition(static_cast<float>(numBlockHorizontal_ / 2), static_cast<float>(numBlockVertical_ / 2));
                }
            } else if (type == "world") {
                hl.startWorldPos.x = item.value("startX", 0.0f);
                hl.startWorldPos.y = item.value("startY", 0.0f);
                hl.startWorldPos.z = item.value("startZ", 0.0f);
            } else { // "index" (マップチップ座標)
                float sx = item.value("startX", 0.0f);
                float sy = item.value("startY", 0.0f);
                hl.startWorldPos = GetMapChipPosition(sx, sy);
            }

            highlights_.push_back(hl);
        }
    }

    // ハイライトが未定義でゴールがある場合は自動でゴールをハイライト
    if (highlights_.empty() && hasGoal_) {
        StageHighlightData hl;
        hl.name = "Goal";
        hl.startWorldPos = goalPosition_;
        hl.panDirection = { 0.0f, 1.5f, 0.0f };
        hl.displayDuration = 2.5f;
        highlights_.push_back(hl);
    }

    return true;
}

void MapChipField::LoadmapChipCsv(const std::string& filePath) {
    ResetMapChipData();

    std::ifstream file;
    file.open(filePath);
    assert(file.is_open());

    std::string line;
    bool readingTerrain = true;

    // ファイルから1行ずつ読み込む（終端までループ）
    while (getline(file, line)) {
        // 改行コード(\r)が末尾に残る場合への対策（Windows環境などの安全策）
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // 空行があった場合
        if (line.empty()) {
            // 地形読み込み中に空行が来たら、オブジェクト読み込みに移行
            if (readingTerrain && !mapChipData_.data.empty()) {
                readingTerrain = false;
            }
            continue;
        }

        if (readingTerrain) {
            std::vector<MapChipType> mapChipLine;
            std::istringstream line_stream(line);
            std::string word;

            // カンマ区切りで各ブロックを読み込む
            while (getline(line_stream, word, ',')) {
                if (!word.empty() && word.back() == '\r') {
                    word.pop_back();
                }

                if (mapChipTable.contains(word)) {
                    MapChipType chipType = mapChipTable[word];
                    mapChipLine.push_back(chipType);
                    if (chipType == MapChipType::kGoal) {
                        hasGoal_ = true;
                    }
                } else {
                    mapChipLine.push_back(MapChipType::kBlank);
                }
            }
            mapChipData_.data.push_back(mapChipLine);
            numBlockHorizontal_ = std::max(numBlockHorizontal_, static_cast<uint32_t>(mapChipLine.size()));
        } else {
            // オブジェクト読み込みフェーズ
            // 期待するフォーマット： normalenemy(14,9) または nogravityenemy(3.5, 2.0)
            size_t openParenthesis = line.find('(');
            size_t closeParenthesis = line.find(')');
            if (openParenthesis != std::string::npos && closeParenthesis != std::string::npos && closeParenthesis > openParenthesis) {
                std::string type = line.substr(0, openParenthesis);
                
                // トリミング
                type.erase(std::remove_if(type.begin(), type.end(), ::isspace), type.end());

                std::string coordsStr = line.substr(openParenthesis + 1, closeParenthesis - openParenthesis - 1);
                std::istringstream coordStream(coordsStr);
                std::string xStr, yStr;
                if (getline(coordStream, xStr, ',') && getline(coordStream, yStr)) {
                    float x = std::stof(xStr);
                    float y = std::stof(yStr);
                    enemySpawns_.push_back({type, x, y});
                }
            }
        }
    }

    file.close();

    // 縦幅（行数）を設定
    numBlockVertical_ = static_cast<uint32_t>(mapChipData_.data.size());

    // 各行の要素数がバラバラだとバグの原因になるため、一番長い行（列数）に合わせて空白で埋める
    for (auto& mapChipLine : mapChipData_.data) {
        if (mapChipLine.size() < numBlockHorizontal_) {
            mapChipLine.resize(numBlockHorizontal_, MapChipType::kBlank);
        }
    }

    if (hasGoal_) {
        for (uint32_t y = 0; y < numBlockVertical_; ++y) {
            for (uint32_t x = 0; x < numBlockHorizontal_; ++x) {
                if (mapChipData_.data[y][x] == MapChipType::kGoal) {
                    goalPosition_ = GetMapChipPositionByIndex(x, y);
                    break;
                }
            }
        }
    }
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) const {
    // メンバ変数に変更した numBlockHorizontal_ と numBlockVertical_ で範囲チェック
    if (xIndex >= numBlockHorizontal_ || yIndex >= numBlockVertical_) {
        return MapChipType::kBlank;
    }

    return mapChipData_.data[yIndex][xIndex];
}

Vector3 MapChipField::GetMapChipPosition(float x, float y) const {
    return Vector3(kBlockWidth * x, kBlockHeight * (static_cast<float>(numBlockVertical_) - 1.0f - y), 0.0f);
}

Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) const {
    return GetMapChipPosition(static_cast<float>(xIndex), static_cast<float>(yIndex));
}

IndexSet MapChipField::GetMapChipIndexSetByPosition(const Vector3& position) const {
    IndexSet indexSet = {};
    indexSet.xIndex = uint32_t((position.x + kBlockWidth / 2.0f) / kBlockWidth);
    // メンバ変数 numBlockVertical_ を使用
    indexSet.yIndex = uint32_t(numBlockVertical_ - ((position.y + kBlockHeight / 2.0f) / kBlockHeight));
    return indexSet;
}

Rect MapChipField::GetRectByIndex(uint32_t xIndex, uint32_t yIndex) const {
    Vector3 center = GetMapChipPositionByIndex(xIndex, yIndex);

    Rect rect;
    rect.left = center.x + kBlockWidth / 2.0f;
    rect.right = center.x - kBlockWidth / 2.0f;
    rect.top = center.y + kBlockHeight / 2.0f;
    rect.bottom = center.y - kBlockHeight / 2.0f;

    return rect;
}

IndexSet MapChipField::GetMapChipIndexSetByCenter() const {
    IndexSet centerIndex = {};

    // 横幅（列数）の半分を中心とする
    centerIndex.xIndex = numBlockHorizontal_ / 2;

    // 縦幅（行数）の半分を中心とする
    centerIndex.yIndex = numBlockVertical_ / 2;

    return centerIndex;
}
