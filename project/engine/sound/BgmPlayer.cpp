#include "BgmPlayer.h"
#include "SoundManager.h"
#include <cstring>

namespace {
    bool IsSameFormat(const WAVEFORMATEXTENSIBLE& a, const WAVEFORMATEXTENSIBLE& b) {
        return std::memcmp(&a, &b, sizeof(WAVEFORMATEXTENSIBLE)) == 0;
    }
}

BgmPlayer* BgmPlayer::GetInstance() {
    static BgmPlayer instance;
    return &instance;
}

BgmPlayer::~BgmPlayer() {
    Finalize();
}

void BgmPlayer::Initialize() {
    if (isInitialized_) {
        return;
    }
    isInitialized_ = true;
}

void BgmPlayer::Finalize() {
    if (!isInitialized_) {
        return;
    }

    Stop();

    if (sourceVoice_) {
        sourceVoice_->DestroyVoice();
        sourceVoice_ = nullptr;
    }
    hasFormat_ = false;
    currentSoundId_ = 0;
    isPlaying_ = false;
    isPaused_ = false;
    isInitialized_ = false;
}

bool BgmPlayer::Play(uint32_t soundId, bool loop, float volume) {
    if (!isInitialized_) {
        Initialize();
    }

    SoundManager* soundMgr = SoundManager::GetInstance();
    const SoundData* soundData = soundMgr->GetSoundData(soundId);
    if (!soundData || soundData->buffer.empty()) {
        return false;
    }

    IXAudio2* xAudio2 = soundMgr->GetXAudio2();
    if (!xAudio2) {
        return false;
    }

    // 既に再生中の場合は停止
    Stop();

    // フォーマット変更が必要か確認
    if (!sourceVoice_ || !hasFormat_ || !IsSameFormat(currentFormat_, soundData->format)) {
        if (sourceVoice_) {
            sourceVoice_->DestroyVoice();
            sourceVoice_ = nullptr;
        }

        HRESULT hr = xAudio2->CreateSourceVoice(
            &sourceVoice_,
            reinterpret_cast<const WAVEFORMATEX*>(&soundData->format)
        );
        if (FAILED(hr) || !sourceVoice_) {
            hasFormat_ = false;
            return false;
        }
        currentFormat_ = soundData->format;
        hasFormat_ = true;
    }

    XAUDIO2_BUFFER xBuffer = {};
    xBuffer.pAudioData = soundData->buffer.data();
    xBuffer.AudioBytes = static_cast<UINT32>(soundData->buffer.size());
    xBuffer.Flags = XAUDIO2_END_OF_STREAM;
    xBuffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

    HRESULT hr = sourceVoice_->SubmitSourceBuffer(&xBuffer);
    if (FAILED(hr)) {
        return false;
    }

    currentTrackVolume_ = (volume < 0.0f) ? 0.0f : volume;
    sourceVoice_->SetVolume(currentTrackVolume_ * bgmVolume_);

    hr = sourceVoice_->Start(0);
    if (FAILED(hr)) {
        return false;
    }

    currentSoundId_ = soundId;
    isLoop_ = loop;
    isPlaying_ = true;
    isPaused_ = false;

    return true;
}

bool BgmPlayer::Play(const std::string& filePath, bool loop, float volume) {
    uint32_t soundId = SoundManager::GetInstance()->LoadSound(filePath);
    if (soundId == 0) {
        return false;
    }
    return Play(soundId, loop, volume);
}

void BgmPlayer::Stop() {
    if (sourceVoice_) {
        sourceVoice_->Stop(0);
        sourceVoice_->FlushSourceBuffers();
    }
    isPlaying_ = false;
    isPaused_ = false;
}

void BgmPlayer::Pause() {
    if (sourceVoice_ && isPlaying_ && !isPaused_) {
        sourceVoice_->Stop(0);
        isPaused_ = true;
    }
}

void BgmPlayer::Resume() {
    if (sourceVoice_ && isPlaying_ && isPaused_) {
        sourceVoice_->Start(0);
        isPaused_ = false;
    }
}

bool BgmPlayer::IsPlaying() const {
    if (!sourceVoice_ || !isPlaying_ || isPaused_) {
        return false;
    }
    XAUDIO2_VOICE_STATE state;
    sourceVoice_->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
    return state.BuffersQueued > 0;
}

void BgmPlayer::SetVolume(float volume) {
    bgmVolume_ = (volume < 0.0f) ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    if (sourceVoice_) {
        sourceVoice_->SetVolume(currentTrackVolume_ * bgmVolume_);
    }
}
