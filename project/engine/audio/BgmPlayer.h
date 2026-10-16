#pragma once
#include <string>
#include <cstdint>
#include <xaudio2.h>
#include "SoundData.h"

/// <summary>
/// BGM専用プレイヤー
/// シームレスループ再生、一時停止、再開、BGM音量制御を提供
/// </summary>
class BgmPlayer {
public:
    static BgmPlayer* GetInstance();

    void Initialize();
    void Finalize();

    /// <summary>
    /// BGMを再生
    /// </summary>
    /// <param name="soundId">SoundManagerでロードしたサウンドID</param>
    /// <param name="loop">ループ再生するかどうか (デフォルト true)</param>
    /// <param name="volume">音量倍率 (0.0f - 1.0f)</param>
    /// <returns>成功したかどうか</returns>
    bool Play(uint32_t soundId, bool loop = true, float volume = 1.0f);

    /// <summary>
    /// ファイルパスを指定して直接BGMを再生
    /// </summary>
    bool Play(const std::string& filePath, bool loop = true, float volume = 1.0f);

    /// <summary>
    /// 再生停止
    /// </summary>
    void Stop();

    /// <summary>
    /// 一時停止
    /// </summary>
    void Pause();

    /// <summary>
    /// 再開
    /// </summary>
    void Resume();

    /// <summary>
    /// 再生中かどうか
    /// </summary>
    bool IsPlaying() const;

    /// <summary>
    /// 一時停止中かどうか
    /// </summary>
    bool IsPaused() const { return isPaused_; }

    /// <summary>
    /// BGMマスター音量設定 (0.0f - 1.0f)
    /// </summary>
    void SetVolume(float volume);

    /// <summary>
    /// BGMマスター音量取得
    /// </summary>
    float GetVolume() const { return bgmVolume_; }

    BgmPlayer(const BgmPlayer&) = delete;
    BgmPlayer& operator=(const BgmPlayer&) = delete;

private:
    BgmPlayer() = default;
    ~BgmPlayer();

    IXAudio2SourceVoice* sourceVoice_ = nullptr;
    WAVEFORMATEXTENSIBLE currentFormat_ = {};
    bool hasFormat_ = false;

    uint32_t currentSoundId_ = 0;
    float currentTrackVolume_ = 1.0f;
    float bgmVolume_ = 1.0f;
    bool isPlaying_ = false;
    bool isPaused_ = false;
    bool isLoop_ = true;
    bool isInitialized_ = false;
};
