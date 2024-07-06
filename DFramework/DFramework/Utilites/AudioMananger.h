#pragma once
#include "../pch.h"
#include "Audio.h"

namespace FDW
{

	class AudioMananger // TO DO MP3 support 
	{

	public:

		AudioMananger();
		~AudioMananger()=default;

		Audio* CreateAudio(const std::wstring& path);

	private:

		void InitXAudio();
		
		std::unique_ptr<IXAudio2> pAudio;
		std::unique_ptr<IXAudio2MasteringVoice> pMasterVoice;

	private:
		//			WAV	CHUNKS
		static HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition);
		static HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset);

	};






}