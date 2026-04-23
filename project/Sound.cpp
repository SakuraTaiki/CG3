#include "Sound.h"
#include <cassert>
#include <cstring>

using namespace Microsoft::WRL;

void Sound::Initialize() {
    HRESULT result;

    // XAudio2エンジンのインスタンスを生成
    result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    assert(SUCCEEDED(result));

    // マスターボイスを生成
    result = xAudio2->CreateMasteringVoice(&masterVoice);
    assert(SUCCEEDED(result));
}

void Sound::Finalize() {
    // masterVoiceを破棄
    if (masterVoice) {
        masterVoice->DestroyVoice();
        masterVoice = nullptr;
    }

    // xAudio2を解放
    xAudio2.Reset();

    // 読み込んだ音声データの解放
    for (SoundData& soundData : soundDatas) {
        SoundUnload(&soundData);
    }
    soundDatas.clear();
}

Sound::SoundData Sound::SoundLoadWave(const char* filename) {
    // ファイル入力ストリームのインスタンス
    std::ifstream file;
    // .wavファイルをバイナリモードで開く
    file.open(filename, std::ios_base::binary);
    // ファイルオープン失敗を検出する
    assert(file.is_open());

    // RIFFヘッダの読み込み
    RiffHeader riff;
    file.read((char*)&riff, sizeof(riff));

    // ファイルがRIFFかチェック
    if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
        assert(0);
    }

    // タイプがWAVEかチェック
    if (strncmp(riff.type, "WAVE", 4) != 0) {
        assert(0);
    }

    // fmtチャンクの読み込み
    FormatChunk format = {};
    file.read((char*)&format.chunk, sizeof(ChunkHeader));

    if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
        assert(0);
    }

    assert(format.chunk.size <= sizeof(format.fmt));
    file.read((char*)&format.fmt, format.chunk.size);

    // dataチャンクの読み込み
    ChunkHeader data;
    file.read((char*)&data, sizeof(data));

    // JUNKチャンクを検出した場合
    if (strncmp(data.id, "JUNK", 4) == 0) {
        // 読み取り位置をJUNKチャンクの終わりまで進める
        file.seekg(data.size, std::ios_base::cur);
        // 再読み込み
        file.read((char*)&data, sizeof(data));
    }

    if (strncmp(data.id, "data", 4) != 0) {
        assert(0);
    }

    // dataチャンクのデータ部(波形データ)の読み込み
    char* pBuffer = new char[data.size];
    file.read(pBuffer, data.size);

    // waveファイルを閉じる
    file.close();

    // returnする為の音声データ
    SoundData soundData = {};

    soundData.wfex = format.fmt;
    soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
    soundData.bufferSize = data.size;

    // 読み込んだ音声データを保持
    soundDatas.push_back(soundData);

    return soundData;
}

void Sound::SoundUnload(SoundData* soundData) {
    // バッファのメモリを解放
    delete[] soundData->pBuffer;

    soundData->pBuffer = 0;
    soundData->bufferSize = 0;
    soundData->wfex = {};
}

void Sound::SoundPlayWave(const SoundData& soundData) {
    HRESULT result;

    // 波形フォーマットを元にSourceVoiceの生成
    IXAudio2SourceVoice* pSourceVoice = nullptr;
    result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
    assert(SUCCEEDED(result));

    // 再生する波形データの設定
    XAUDIO2_BUFFER buf{};
    buf.pAudioData = soundData.pBuffer;
    buf.AudioBytes = soundData.bufferSize;
    buf.Flags = XAUDIO2_END_OF_STREAM;

    // 波形データの再生
    result = pSourceVoice->SubmitSourceBuffer(&buf);
    assert(SUCCEEDED(result));

    result = pSourceVoice->Start();
    assert(SUCCEEDED(result));
}