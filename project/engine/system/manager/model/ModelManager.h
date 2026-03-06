#pragma once
#include <map>
#include <string>
#include <memory>
#include "ModelBuilder.h"

class ModelManager final
{
public:
    // シングルトンパターン
    static ModelManager* GetInstance();

    // モデルデータのロードまたは取得
    // ロード済みの場合はキャッシュから返し、未ロードの場合は新しくロードする
    const ModelData& LoadModel(const std::string& directoryPath, const std::string& filename);

    // リソースの解放
    void Finalize();

private:
    // コンストラクタをprivateにしてシングルトンを強制
    ModelManager() = default;
    ~ModelManager() = default;
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    // モデルデータのキャッシュ
    std::map<std::string, std::unique_ptr<ModelData>> modelDataCache_;

    // モデルデータのキー生成
    std::string CreateModelKey(const std::string& directoryPath, const std::string& filename);
};