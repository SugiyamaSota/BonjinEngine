#include "MapChipField.h"

#include <map>
#include <fstream>
#include <sstream>
#include <cassert>
#include <algorithm> // std::max用

namespace {
    std::map<std::string, MapChipType> mapChipTable = {
        {"0", MapChipType::kBlank},
        {"1", MapChipType::kBlock},
        {"2", MapChipType::kGoal},
        {"G", MapChipType::kGoal},
    };
}

void MapChipField::ResetMapChipData() {
    mapChipData_.data.clear();
    enemySpawns_.clear();
    numBlockVertical_ = 0;
    numBlockHorizontal_ = 0;
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
                    mapChipLine.push_back(mapChipTable[word]);
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
