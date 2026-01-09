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

    // 全リソースの解放 (アプリケーション終了時など)
    void Finalize();

private:
    // コンストラクタをprivateにしてシングルトンを強制
    ModelManager() = default;
    ~ModelManager() = default;
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

private:
    // モデルデータのキャッシュ
    // キー: モデルの識別子 (例: "sphere.obj")
    // 値: モデルデータ (ModelData)
    // ModelDataはコピーされるか、shared_ptr/unique_ptrで保持されます。
    // 今回は単純なコピーではなく、ポインタ（またはスマートポインタ）で実体を保持し、
    // 参照を返す設計にします。
    std::map<std::string, std::unique_ptr<ModelData>> modelDataCache_;

    // モデルデータのキー生成 (例: "resources/models/sphere.obj" -> "sphere.obj")
    std::string CreateModelKey(const std::string& directoryPath, const std::string& filename);
};