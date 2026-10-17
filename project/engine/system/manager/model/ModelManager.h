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
    // キーを指定してモデルをロードしてキャッシュに登録（起動時ロード用）
    void LoadModel(const std::string& key, const std::string& directoryPath, const std::string& filename);

    // ロード済みの場合はキャッシュから返し、未ロードの場合は新しくロードする（ファイル名がキー）
    const ModelData& LoadModel(const std::string& directoryPath, const std::string& filename);

    // 登録されたキーからモデルデータを取得
    const ModelData& GetModel(const std::string& key) const;

    // キーが登録済みか確認
    bool HasModel(const std::string& key) const;

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

    // ファイル名等から登録キーへのエイリアスマップ
    std::map<std::string, std::string> keyAliases_;

    // モデルデータのキー生成
    std::string CreateModelKey(const std::string& directoryPath, const std::string& filename);
};