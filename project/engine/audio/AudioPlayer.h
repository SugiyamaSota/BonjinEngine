#pragma once
#include<string>
#include <xaudio2.h>
#include <wrl/client.h>
#include "../../externals/Audio/common.h"
#include"../../externals/Audio/xaudio_player_core.h"
#include"../../externals/Audio/xaudio_player_mf.h"

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    HRESULT Initialize(const std::wstring& filePath);
    void Start();
    void Stop();
    bool IsPlaying() const;
    void Finalize();

private:
    struct mf_initializer {
    private:
        HRESULT _hr;
    public:
        mf_initializer() : _hr(::MFStartup(MF_VERSION, 0)) {}
        ~mf_initializer() { if (SUCCEEDED(_hr)) { ::MFShutdown(); } }
    };
    mf_initializer mfinit_local;

    Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
    IXAudio2MasteringVoice* masteringVoice_;
    xaudio_player_mf player_;
    bool isInitialized_;
    bool isPlaying_;

    co_initializer coinit_;
};