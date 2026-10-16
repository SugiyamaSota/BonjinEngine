#include "SePlayer.h"
#include "SoundManager.h"
#include <cstring>
#include <limits>

namespace {
    bool IsSameFormat(const WAVEFORMATEXTENSIBLE& a, const WAVEFORMATEXTENSIBLE& b) {
        return std::memcmp(&a, &b, sizeof(WAVEFORMATEXTENSIBLE)) == 0;
    }
}

SePlayer* SePlayer::GetInstance() {
    static SePlayer instance;
    return &instance;
}

SePlayer::~SePlayer() {
    Finalize();
}

void SePlayer::Initialize() {
    if (isInitialized_) {
        return;
    }

    voicePool_.clear();
    voicePool_.resize(kMaxVoices);

    isInitialized_ = true;
}

void SePlayer::Finalize() {
    if (!isInitialized_) {
        return;
    }

    StopAll();

    for (auto& slot : voicePool_) {
        if (slot.sourceVoice) {
            slot.sourceVoice->DestroyVoice();
            slot.sourceVoice = nullptr;
        }
        slot.hasFormat = false;
        slot.inUse = false;
    }
    voicePool_.clear();

    isInitialized_ = false;
}

void SePlayer::Update() {
    if (!isInitialized_) return;

    for (auto& slot : voicePool_) {
        if (slot.inUse && slot.sourceVoice) {
            XAUDIO2_VOICE_STATE state;
            slot.sourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
            if (state.BuffersQueued == 0) {
                // 再生完了
                slot.inUse = false;
            }
        }
    }
}

int SePlayer::Play(uint32_t soundId, float volume, float pitchRatio) {
    if (!isInitialized_) {
        Initialize();
    }

    SoundManager* soundMgr = SoundManager::GetInstance();
    const SoundData* soundData = soundMgr->GetSoundData(soundId);
    if (!soundData || soundData->buffer.empty()) {
        return -1;
    }

    IXAudio2* xAudio2 = soundMgr->GetXAudio2();
    if (!xAudio2) {
        return -1;
    }

    // 更新して終了済みボイスを回収
    Update();

    // 空きスロット探索
    int targetIndex = -1;
    for (size_t i = 0; i < voicePool_.size(); ++i) {
        if (!voicePool_[i].inUse) {
            targetIndex = static_cast<int>(i);
            break;
        }
    }

    // 空きがなければ最も古く再生開始されたボイスをスティール（上書き利用）
    if (targetIndex == -1) {
        uint64_t oldestTimestamp = (std::numeric_limits<uint64_t>::max)();
        for (size_t i = 0; i < voicePool_.size(); ++i) {
            if (voicePool_[i].playTimestamp < oldestTimestamp) {
                oldestTimestamp = voicePool_[i].playTimestamp;
                targetIndex = static_cast<int>(i);
            }
        }
    }

    if (targetIndex < 0 || targetIndex >= static_cast<int>(voicePool_.size())) {
        return -1;
    }

    VoiceSlot& slot = voicePool_[targetIndex];

    // フォーマット変更が必要か確認
    if (!slot.sourceVoice || !slot.hasFormat || !IsSameFormat(slot.currentFormat, soundData->format)) {
        if (slot.sourceVoice) {
            slot.sourceVoice->Stop(0);
            slot.sourceVoice->DestroyVoice();
            slot.sourceVoice = nullptr;
        }

        HRESULT hr = xAudio2->CreateSourceVoice(
            &slot.sourceVoice,
            reinterpret_cast<const WAVEFORMATEX*>(&soundData->format)
        );
        if (FAILED(hr) || !slot.sourceVoice) {
            slot.inUse = false;
            slot.hasFormat = false;
            return -1;
        }
        slot.currentFormat = soundData->format;
        slot.hasFormat = true;
    } else {
        slot.sourceVoice->Stop(0);
        slot.sourceVoice->FlushSourceBuffers();
    }

    // XAUDIO2_BUFFER を設定
    XAUDIO2_BUFFER xBuffer = {};
    xBuffer.pAudioData = soundData->buffer.data();
    xBuffer.AudioBytes = static_cast<UINT32>(soundData->buffer.size());
    xBuffer.Flags = XAUDIO2_END_OF_STREAM;

    HRESULT hr = slot.sourceVoice->SubmitSourceBuffer(&xBuffer);
    if (FAILED(hr)) {
        slot.inUse = false;
        return -1;
    }

    slot.individualVolume = (volume < 0.0f) ? 0.0f : volume;
    slot.sourceVoice->SetVolume(slot.individualVolume * seVolume_);

    if (pitchRatio > 0.0f && pitchRatio <= XAUDIO2_MAX_FREQ_RATIO) {
        slot.sourceVoice->SetFrequencyRatio(pitchRatio);
    }

    hr = slot.sourceVoice->Start(0);
    if (FAILED(hr)) {
        slot.inUse = false;
        return -1;
    }

    slot.inUse = true;
    slot.currentSoundId = soundId;
    slot.playTimestamp = nextPlayTimestamp_++;

    return targetIndex;
}

int SePlayer::Play(const std::string& filePath, float volume, float pitchRatio) {
    uint32_t soundId = SoundManager::GetInstance()->LoadSound(filePath);
    if (soundId == 0) {
        return -1;
    }
    return Play(soundId, volume, pitchRatio);
}

void SePlayer::Stop(int voiceIndex) {
    if (voiceIndex < 0 || voiceIndex >= static_cast<int>(voicePool_.size())) {
        return;
    }
    VoiceSlot& slot = voicePool_[voiceIndex];
    if (slot.sourceVoice) {
        slot.sourceVoice->Stop(0);
        slot.sourceVoice->FlushSourceBuffers();
    }
    slot.inUse = false;
}

void SePlayer::StopAll() {
    for (auto& slot : voicePool_) {
        if (slot.sourceVoice) {
            slot.sourceVoice->Stop(0);
            slot.sourceVoice->FlushSourceBuffers();
        }
        slot.inUse = false;
    }
}

bool SePlayer::IsPlaying(int voiceIndex) const {
    if (voiceIndex < 0 || voiceIndex >= static_cast<int>(voicePool_.size())) {
        return false;
    }
    const VoiceSlot& slot = voicePool_[voiceIndex];
    if (!slot.inUse || !slot.sourceVoice) {
        return false;
    }
    XAUDIO2_VOICE_STATE state;
    slot.sourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
    return state.BuffersQueued > 0;
}

void SePlayer::SetVolume(float volume) {
    seVolume_ = (volume < 0.0f) ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    for (auto& slot : voicePool_) {
        if (slot.sourceVoice) {
            slot.sourceVoice->SetVolume(slot.individualVolume * seVolume_);
        }
    }
}
