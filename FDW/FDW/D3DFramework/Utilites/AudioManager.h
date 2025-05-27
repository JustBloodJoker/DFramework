#pragma once
#include "../pch.h"
#include "Audio.h"

namespace FD3DW
{

	class AudioManager // TO DO MP3 support 
	{

	public:

		AudioManager();
		~AudioManager()=default;

		Audio* CreateAudio(const std::wstring& path);

	private:

		void InitXAudio();
		
		std::unique_ptr<IXAudio2> m_pAudio;
		std::unique_ptr<IXAudio2MasteringVoice> m_pMasterVoice;

	private:
		//			WAV	CHUNKS
		static HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition);
		static HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset);

	};






}