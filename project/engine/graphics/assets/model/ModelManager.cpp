#include "ModelManager.h"
#include <cassert>

ModelManager* ModelManager::GetInstance()
{
    static ModelManager instance;
    return &instance;
}

std::string ModelManager::CreateModelKey(const std::string& directoryPath, const std::string& filename)
{
    // シンプルにファイル名のみをキーとする
    return filename;
}

const ModelData& ModelManager::LoadModel(const std::string& directoryPath, const std::string& filename)
{
    std::string key = CreateModelKey(directoryPath, filename);

    // キャッシュをチェック
    if (modelDataCache_.count(key)) {
        return *modelDataCache_.at(key);
    }

    // 未ロードの場合、新しくロード
    ModelData loadedData = ModelBuilder::LoadObjFile(directoryPath, filename);

    // キャッシュに保存
    modelDataCache_[key] = std::make_unique<ModelData>(std::move(loadedData));

    // ロードしたデータを返す
    return *modelDataCache_.at(key);
}

void ModelManager::Finalize()
{
    modelDataCache_.clear();
}