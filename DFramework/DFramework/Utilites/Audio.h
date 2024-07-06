#pragma once
#include "../pch.h"

namespace FDW
{


	class Audio  // TO DO AUDIO EFFECTS
	{

	public:

		Audio() = delete;
		Audio(IXAudio2SourceVoice* source);
		~Audio() = default;

		void SetVolume(float volume);
		void Play(UINT flags = 0u);
		void Stop(UINT flags = 0u);
		
	
	private:

		IXAudio2SourceVoice* pSourceVoice;

	};


}
