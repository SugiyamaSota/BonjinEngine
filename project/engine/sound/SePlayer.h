#pragma once
#include <vector>
#include <cstdint>
#include <xaudio2.h>
#include "SoundData.h"

/// <summary>
/// SE（効果音）専用プレイヤー
/// 多重再生（ボイスプール）に対応し、同一音や複数音の重奏を実現
/// </summary>
class SePlayer {
public:
    static constexpr size_t kMaxVoices = 32;

    static SePlayer* GetInstance();

    void Initialize();
    void Finalize();

    /// <summary>
    /// フレーム更新処理（再生終了ボイスの回収）
    /// </summary>
    void Update();

    /// <summary>
    /// SEを再生
    /// </summary>
    /// <param name="soundId">SoundManagerでロードしたサウンドID</param>
    /// <param name="volume">音量倍率 (0.0f - 1.0f)</param>
    /// <param name="pitchRatio">ピッチ倍率 (デフォルト1.0f)</param>
    /// <returns>再生ボイススロットインデックス（失敗時 -1）</returns>
    int Play(uint32_t soundId, float volume = 1.0f, float pitchRatio = 1.0f);

    /// <summary>
    /// ファイルパスを指定して直接再生（未ロードなら自動ロード）
    /// </summary>
    int Play(const std::string& filePath, float volume = 1.0f, float pitchRatio = 1.0f);

    /// <summary>
    /// 特定スロットの再生停止
    /// </summary>
    void Stop(int voiceIndex);

    /// <summary>
    /// 再生中のすべてのSEを停止
    /// </summary>
    void StopAll();

    /// <summary>
    /// 再生中かどうか判定
    /// </summary>
    bool IsPlaying(int voiceIndex) const;

    /// <summary>
    /// SEマスター音量設定 (0.0f - 1.0f)
    /// </summary>
    void SetVolume(float volume);

    /// <summary>
    /// SEマスター音量取得
    /// </summary>
    float GetVolume() const { return seVolume_; }

    SePlayer(const SePlayer&) = delete;
    SePlayer& operator=(const SePlayer&) = delete;

private:
    SePlayer() = default;
    ~SePlayer();

    struct VoiceSlot {
        IXAudio2SourceVoice* sourceVoice = nullptr;
        WAVEFORMATEXTENSIBLE currentFormat = {};
        bool hasFormat = false;
        uint32_t currentSoundId = 0;
        bool inUse = false;
        uint64_t playTimestamp = 0;
        float individualVolume = 1.0f;
    };

    std::vector<VoiceSlot> voicePool_;
    float seVolume_ = 1.0f;
    uint64_t nextPlayTimestamp_ = 1;
    bool isInitialized_ = false;
};
