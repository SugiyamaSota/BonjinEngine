#include "ModelManager.h"
#include "TextureManager.h"
#include "DirectXCommon.h"
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

void ModelManager::LoadModel(const std::string& key, const std::string& directoryPath, const std::string& filename)
{
    // 既にロード済みならスキップ
    if (modelDataCache_.count(key)) {
        return;
    }

    // 未ロードの場合、新しくロード
    ModelData loadedData = ModelBuilder::LoadModelFile(directoryPath, filename);

    // テクスチャがあれば事前にロードしておく（起動時にテクスチャもロード完了）
    if (!loadedData.material.textureFilepath.empty()) {
        TextureManager::GetInstance()->LoadTexture(loadedData.material.textureFilepath);
        DirectXCommon::GetInstance()->WaitAndResetCommandList();
        TextureManager::GetInstance()->ReleaseIntermediateResources();
    }

    // キャッシュに保存
    modelDataCache_[key] = std::make_unique<ModelData>(std::move(loadedData));

    // ファイル名でも引けるようにエイリアスを登録
    if (key != filename) {
        keyAliases_[filename] = key;
    }
}

const ModelData& ModelManager::LoadModel(const std::string& directoryPath, const std::string& filename)
{
    std::string key = CreateModelKey(directoryPath, filename);

    // キャッシュをチェック
    if (modelDataCache_.count(key)) {
        return *modelDataCache_.at(key);
    }
    // エイリアスをチェック
    auto aliasIt = keyAliases_.find(key);
    if (aliasIt != keyAliases_.end() && modelDataCache_.count(aliasIt->second)) {
        return *modelDataCache_.at(aliasIt->second);
    }

    // 未ロードの場合、新しくロード
    LoadModel(key, directoryPath, filename);

    return *modelDataCache_.at(key);
}

const ModelData& ModelManager::GetModel(const std::string& key) const
{
    auto it = modelDataCache_.find(key);
    if (it != modelDataCache_.end()) {
        return *(it->second);
    }

    // エイリアス（ファイル名など）から検索
    auto aliasIt = keyAliases_.find(key);
    if (aliasIt != keyAliases_.end()) {
        auto aliasModelIt = modelDataCache_.find(aliasIt->second);
        if (aliasModelIt != modelDataCache_.end()) {
            return *(aliasModelIt->second);
        }
    }

    assert(false && "Specified model key was not found in ModelManager! Make sure it was loaded at startup.");
    static ModelData emptyData;
    return emptyData;
}

bool ModelManager::HasModel(const std::string& key) const
{
    if (modelDataCache_.count(key) > 0) {
        return true;
    }
    auto aliasIt = keyAliases_.find(key);
    if (aliasIt != keyAliases_.end() && modelDataCache_.count(aliasIt->second) > 0) {
        return true;
    }
    return false;
}

void ModelManager::Finalize()
{
    modelDataCache_.clear();
    keyAliases_.clear();
}