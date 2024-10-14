#include "../pch.h"
#include "AudioMananger.h"

namespace FD3DW
{


	AudioMananger::AudioMananger()
	{
		InitXAudio();
	}

    Audio* AudioMananger::CreateAudio(const std::wstring& path)
    {
        HANDLE hFile = CreateFile(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (INVALID_HANDLE_VALUE == hFile)
            HRESULT_ASSERT(HRESULT_FROM_WIN32(GetLastError()), "Can't open audio file");

        if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
            HRESULT_ASSERT(HRESULT_FROM_WIN32(GetLastError()), "Can't set file ptr to begin");

        WAVEFORMATEXTENSIBLE wfx = { 0 };
        XAUDIO2_BUFFER buffer = { 0 };
        IXAudio2SourceVoice* sourceVoice;
        DWORD dwChunkSize;
        DWORD dwChunkPosition;
        
        //RIFF   size
        HRESULT_ASSERT(FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition), "Find chunk error");
        DWORD filetype; 
        HRESULT_ASSERT(ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition), "Read chunk error");
        if (filetype != fourccWAVE) 
            SAFE_ASSERT(0, "Audio file isn't WAVE");

        //WAVE  " fmt"  size formatData  
        HRESULT_ASSERT(FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition), "Find chunk error");
        HRESULT_ASSERT(ReadChunkData(hFile, &wfx, dwChunkSize, dwChunkPosition), "Read chunk error");

        //DATA size 
        HRESULT_ASSERT(FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition), "Find chunk error");
        BYTE* pDataBuffer = new BYTE[dwChunkSize];
        HRESULT_ASSERT(ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition), "Read chunk error");;

        buffer.AudioBytes = dwChunkSize;
        buffer.pAudioData = pDataBuffer;
        buffer.Flags = XAUDIO2_END_OF_STREAM;

        HRESULT_ASSERT(pAudio->CreateSourceVoice(&sourceVoice, (WAVEFORMATEX*)&wfx), "Create Source Voice error");
        HRESULT_ASSERT(sourceVoice->SubmitSourceBuffer(&buffer), "Submit buffer error");

        return new Audio(sourceVoice);
    }

	void AudioMananger::InitXAudio()
	{
		HRESULT_ASSERT(CoInitializeEx(NULL, COINITBASE_MULTITHREADED), "Com init error");

		IXAudio2* tempAudio;
		HRESULT_ASSERT(XAudio2Create(&tempAudio, 0, XAUDIO2_DEFAULT_PROCESSOR), "XAudio2 create error");
		pAudio = std::unique_ptr<IXAudio2>(tempAudio);

		IXAudio2MasteringVoice* tempMasteringVoice;
		HRESULT_ASSERT(pAudio->CreateMasteringVoice(&tempMasteringVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_MAX_SAMPLE_RATE), "MasteringVoice create error");
		pMasterVoice = std::unique_ptr<IXAudio2MasteringVoice>(tempMasteringVoice);
	}

	HRESULT FD3DW::AudioMananger::FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
	{
        hr = S_OK;
        if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
            return HRESULT_FROM_WIN32(GetLastError());

        DWORD dwChunkType;
        DWORD dwChunkDataSize;
        DWORD dwRIFFDataSize = 0;
        DWORD dwFileType;
        DWORD bytesRead = 0;
        DWORD dwOffset = 0;

        while (hr == S_OK)
        {
            DWORD dwRead;
            if (!ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
                HRESULT_ASSERT(HRESULT_FROM_WIN32(GetLastError()), "Read file error");

            if (!ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
                HRESULT_ASSERT(HRESULT_FROM_WIN32(GetLastError()),"Read file error");

            switch (dwChunkType)
            {
            case fourccRIFF:
                dwRIFFDataSize = dwChunkDataSize;
                dwChunkDataSize = 4;
                if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
                    HRESULT_ASSERT(HRESULT_FROM_WIN32(GetLastError()),"Read file error");
                break;

            default:
                if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
                    return HRESULT_FROM_WIN32(GetLastError());
            }

            dwOffset += sizeof(DWORD) * 2;

            if (dwChunkType == fourcc)
            {
                dwChunkSize = dwChunkDataSize;
                dwChunkDataPosition = dwOffset;
                return S_OK;
            }

            dwOffset += dwChunkDataSize;

            if (bytesRead >= dwRIFFDataSize) return S_FALSE;

        }

        return S_OK;
	}
	HRESULT AudioMananger::ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset)
	{
        HRESULT hr = S_OK;
        if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
            return HRESULT_FROM_WIN32(GetLastError());
        DWORD dwRead;
        if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
	}
}