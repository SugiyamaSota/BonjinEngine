#include "AudioPlayer.h"

// 音声関連のライブラリリンク
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "xaudio2.lib")

AudioPlayer::AudioPlayer() :
    coinit_(COINIT_MULTITHREADED), // 明示的に初期化リストに書く
    masteringVoice_(nullptr),
    isInitialized_(false),
    isPlaying_(false)
{
    
}

AudioPlayer::~AudioPlayer() {
    Finalize();
}

HRESULT AudioPlayer::Initialize(const std::wstring& filePath) {
    // Already initialized, finalize first
    if (isInitialized_) {
        Finalize();
    }

    // Initialize XAudio2
    HRESULT hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        return hr;
    }

    hr = xAudio2_->CreateMasteringVoice(&masteringVoice_);
    if (FAILED(hr)) {
        return hr;
    }

    hr = player_.initialize(filePath.c_str());
    if (FAILED(hr)) {
        return hr;
    }
    isInitialized_ = SUCCEEDED(hr);
    return hr;
}

void AudioPlayer::Start() {
    if (isInitialized_ && !isPlaying_) {
        player_.start();
        isPlaying_ = true;
    }
}

void AudioPlayer::Stop() {
    if (isPlaying_) {
        player_.stop();
        isPlaying_ = false;
    }
}

bool AudioPlayer::IsPlaying() const {
    return isPlaying_ && player_.is_playing();
}

void AudioPlayer::Finalize() {
    Stop();
    if (masteringVoice_) {
        masteringVoice_->DestroyVoice();
        masteringVoice_ = nullptr;
    }
    xAudio2_.Reset(); // Release the ComPtr
    player_.finalize();
    isInitialized_ = false;
}