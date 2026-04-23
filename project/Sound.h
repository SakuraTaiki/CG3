#pragma once
#include <xaudio2.h>
#include <wrl.h>
#include <fstream>
#include <vector>
#include <cstdint>

#pragma comment(lib, "xaudio2.lib")

class Sound {
public:
    // 初期化
    void Initialize();

    // 終了処理
    void Finalize();

    // 音声データの構造体
    struct SoundData {
        // 波形フォーマット
        WAVEFORMATEX wfex;
        // バッファの先頭アドレス
        BYTE* pBuffer;
        // バッファのサイズ
        unsigned int bufferSize;
    };

    // 音声読み込み
    SoundData SoundLoadWave(const char* filename);

    // 音声データ解放
    void SoundUnload(SoundData* soundData);

    // 音声再生
    void SoundPlayWave(const SoundData& soundData);

private:
    // チャンクヘッダ
    struct ChunkHeader {
        char id[4];
        int32_t size;
    };

    // RIFFヘッダチャンク
    struct RiffHeader {
        ChunkHeader chunk;
        char type[4];
    };

    // FMTチャンク
    struct FormatChunk {
        ChunkHeader chunk;
        WAVEFORMATEX fmt;
    };

private:
    Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
    IXAudio2MasteringVoice* masterVoice = nullptr;

    // 読み込んだ音声データを保持しておく
    std::vector<SoundData> soundDatas;
};