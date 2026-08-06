#pragma once
#include "../pch.h"
#include "Audio.h"

namespace FD3DW
{
	struct XAudio2Releaser {
		void operator()(IXAudio2* audio) const noexcept {
			if (audio) audio->Release();
		}
	};

	struct XAudio2MasteringVoiceReleaser {
		void operator()(IXAudio2MasteringVoice* voice) const noexcept {
			if (voice) voice->DestroyVoice();
		}
	};

	class AudioManager // TODO: MP3 support 
	{

	public:

		AudioManager();
		~AudioManager();

		Audio* CreateAudio(const std::string& path);
		Audio* CreateAudio(const std::wstring& path);

	private:

		void InitXAudio();
		
		std::unique_ptr<IXAudio2, XAudio2Releaser> m_pAudio;
		std::unique_ptr<IXAudio2MasteringVoice, XAudio2MasteringVoiceReleaser> m_pMasterVoice;
		
		bool m_bCOMInitialized = false;

	private:
		//			WAV	CHUNKS
		static HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition);
		static HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset);

	};






}
