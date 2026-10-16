#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <wrl/client.h>
#include <xaudio2.h>
#include "SoundData.h"

/// <summary>
/// XAudio2エンジン基盤および音声リソース管理シングルトンクラス
/// </summary>
class SoundManager {
public:
    /// <summary>
    /// シングルトンインスタンスの取得
    /// </summary>
    static SoundManager* GetInstance();

    /// <summary>
    /// 初期化（XAudio2、Media Foundation、COMの初期化）
    /// </summary>
    void Initialize();

    /// <summary>
    /// 終了処理（リソース解放、XAudio2破棄）
    /// </summary>
    void Finalize();

    /// <summary>
    /// 音声ファイルのロード（WAV、MP3両対応）
    /// </summary>
    /// <param name="filePath">音声ファイルパス</param>
    /// <returns>soundId（0は失敗）</returns>
    uint32_t LoadSound(const std::string& filePath);

    /// <summary>
    /// サウンドデータの取得
    /// </summary>
    /// <param name="soundId">サウンドID</param>
    /// <returns>SoundDataポインタ（存在しない場合はnullptr）</returns>
    const SoundData* GetSoundData(uint32_t soundId) const;

    /// <summary>
    /// XAudio2インスタンス取得
    /// </summary>
    IXAudio2* GetXAudio2() const { return xAudio2_.Get(); }

    /// <summary>
    /// マスタリングボイス取得
    /// </summary>
    IXAudio2MasteringVoice* GetMasteringVoice() const { return masteringVoice_; }

    /// <summary>
    /// マスター音量設定 (0.0f - 1.0f)
    /// </summary>
    void SetMasterVolume(float volume);

    /// <summary>
    /// マスター音量取得
    /// </summary>
    float GetMasterVolume() const { return masterVolume_; }

    /// <summary>
    /// 初期化済みかどうか
    /// </summary>
    bool IsInitialized() const { return isInitialized_; }

    // コピー・代入禁止
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

private:
    SoundManager() = default;
    ~SoundManager();

    // 内部デコード関数（Media Foundation利用）
    bool LoadAudioFileInternal(const std::string& filePath, SoundData& outData);

    Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
    IXAudio2MasteringVoice* masteringVoice_ = nullptr;
    float masterVolume_ = 1.0f;

    bool isInitialized_ = false;
    bool isMfStarted_ = false;
    bool isCoInitialized_ = false;

    // 音声リソースキャッシュ
    std::unordered_map<std::string, uint32_t> pathToIdMap_;
    std::unordered_map<uint32_t, SoundData> soundCache_;
    uint32_t nextSoundId_ = 1;
};
