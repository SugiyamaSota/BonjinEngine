#include "SoundManager.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <iostream>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")

namespace {
    // 文字列をwstringへ変換する補助関数
    std::wstring ConvertToWString(const std::string& str) {
        if (str.empty()) return std::wstring();
        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
        if (sizeNeeded == 0) {
            // UTF-8で失敗した場合はANSI(Shift-JIS)で試す
            sizeNeeded = MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
            std::wstring wstr(sizeNeeded, 0);
            MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.size()), &wstr[0], sizeNeeded);
            return wstr;
        }
        std::wstring wstr(sizeNeeded, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &wstr[0], sizeNeeded);
        return wstr;
    }
}

SoundManager* SoundManager::GetInstance() {
    static SoundManager instance;
    return &instance;
}

SoundManager::~SoundManager() {
    Finalize();
}

void SoundManager::Initialize() {
    if (isInitialized_) {
        return;
    }

    // COMの初期化
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr)) {
        isCoInitialized_ = true;
    }

    // Media Foundationの初期化
    hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    if (SUCCEEDED(hr)) {
        isMfStarted_ = true;
    }

    // XAudio2の生成
    hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        return;
    }

    // マスタリングボイス（出力）の生成
    hr = xAudio2_->CreateMasteringVoice(&masteringVoice_);
    if (FAILED(hr)) {
        xAudio2_.Reset();
        return;
    }

    masteringVoice_->SetVolume(masterVolume_);
    isInitialized_ = true;
}

void SoundManager::Finalize() {
    if (!isInitialized_) {
        return;
    }

    // キャッシュクリア
    soundCache_.clear();
    pathToIdMap_.clear();

    if (masteringVoice_) {
        masteringVoice_->DestroyVoice();
        masteringVoice_ = nullptr;
    }

    xAudio2_.Reset();

    if (isMfStarted_) {
        MFShutdown();
        isMfStarted_ = false;
    }

    if (isCoInitialized_) {
        CoUninitialize();
        isCoInitialized_ = false;
    }

    isInitialized_ = false;
}

uint32_t SoundManager::LoadSound(const std::string& filePath) {
    if (!isInitialized_) {
        Initialize();
    }

    // すでに読み込み済みであればIDを返却
    auto it = pathToIdMap_.find(filePath);
    if (it != pathToIdMap_.end()) {
        return it->second;
    }

    SoundData soundData;
    soundData.filePath = filePath;
    soundData.id = nextSoundId_;

    if (!LoadAudioFileInternal(filePath, soundData)) {
        return 0;
    }

    uint32_t id = nextSoundId_++;
    pathToIdMap_[filePath] = id;
    soundCache_[id] = std::move(soundData);

    return id;
}

const SoundData* SoundManager::GetSoundData(uint32_t soundId) const {
    auto it = soundCache_.find(soundId);
    if (it != soundCache_.end()) {
        return &it->second;
    }
    return nullptr;
}

void SoundManager::SetMasterVolume(float volume) {
    masterVolume_ = (volume < 0.0f) ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    if (masteringVoice_) {
        masteringVoice_->SetVolume(masterVolume_);
    }
}

bool SoundManager::LoadAudioFileInternal(const std::string& filePath, SoundData& outData) {
    std::wstring wFilePath = ConvertToWString(filePath);

    Microsoft::WRL::ComPtr<IMFSourceReader> sourceReader;
    HRESULT hr = MFCreateSourceReaderFromURL(wFilePath.c_str(), nullptr, &sourceReader);
    if (FAILED(hr)) {
        return false;
    }

    // PCMオーディオとして読み込む設定
    Microsoft::WRL::ComPtr<IMFMediaType> pcmAudioType;
    hr = MFCreateMediaType(&pcmAudioType);
    if (FAILED(hr)) return false;

    hr = pcmAudioType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (FAILED(hr)) return false;

    hr = pcmAudioType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    if (FAILED(hr)) return false;

    hr = sourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pcmAudioType.Get());
    if (FAILED(hr)) return false;

    // 実際のフォーマット（WAVEFORMATEXTENSIBLE）を取得
    Microsoft::WRL::ComPtr<IMFMediaType> uncompressedAudioType;
    hr = sourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &uncompressedAudioType);
    if (FAILED(hr)) return false;

    WAVEFORMATEX* pWaveFormat = nullptr;
    UINT32 formatSize = 0;
    hr = MFCreateWaveFormatExFromMFMediaType(uncompressedAudioType.Get(), &pWaveFormat, &formatSize, MFWaveFormatExConvertFlag_ForceExtensible);
    if (FAILED(hr) || pWaveFormat == nullptr) return false;

    outData.format = *reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pWaveFormat);
    CoTaskMemFree(pWaveFormat);

    // PCMデータを順次読み込んで一括バッファに蓄積
    while (true) {
        DWORD streamFlags = 0;
        Microsoft::WRL::ComPtr<IMFSample> sample;
        hr = sourceReader->ReadSample(
            MF_SOURCE_READER_FIRST_AUDIO_STREAM,
            0,
            nullptr,
            &streamFlags,
            nullptr,
            &sample
        );

        if (FAILED(hr) || (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM)) {
            break;
        }

        if (sample) {
            Microsoft::WRL::ComPtr<IMFMediaBuffer> mediaBuffer;
            hr = sample->ConvertToContiguousBuffer(&mediaBuffer);
            if (SUCCEEDED(hr)) {
                BYTE* audioBytes = nullptr;
                DWORD currentLength = 0;
                hr = mediaBuffer->Lock(&audioBytes, nullptr, &currentLength);
                if (SUCCEEDED(hr) && audioBytes != nullptr && currentLength > 0) {
                    outData.buffer.insert(outData.buffer.end(), audioBytes, audioBytes + currentLength);
                    mediaBuffer->Unlock();
                }
            }
        }
    }

    return !outData.buffer.empty();
}
