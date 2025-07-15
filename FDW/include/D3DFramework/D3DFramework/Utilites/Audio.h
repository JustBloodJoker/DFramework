#pragma once
#include "../pch.h"

namespace FD3DW
{


	class Audio  // TO DO USE DIRECT AUDIO
	{

	public:

		Audio() = delete;
		Audio(IXAudio2SourceVoice* source, UINT32 bufferSize, const BYTE* pAudioData);
		~Audio();

		float GetVolume() const;
		void SetVolume(float volume);

		void Restart(UINT playFlags = 0u, UINT endFlags = 0u);
		void Play(UINT flags = 0u);
		void Stop(UINT flags = 0u);
		
		bool IsEnded();

		bool IsLoop();
		void Loop(bool loop, bool force);

	private:
		void ReloadBufferData();
	
	private:
		UINT32 m_uBufferSize;
		const BYTE* m_pAudioData = nullptr;
		IXAudio2SourceVoice* m_pSourceVoice = nullptr;
		bool m_bIsLoop;
		
	};


}
