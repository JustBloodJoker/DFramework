
#include "../pch.h"
#include "Audio.h"

namespace FDW
{


	Audio::Audio(IXAudio2SourceVoice* source)
	{
        this->pSourceVoice = source;
	}

    void Audio::SetVolume(float volume)
    {
        pSourceVoice->SetVolume(volume);
    }

    void Audio::Stop(UINT flags)
    {
        pSourceVoice->Stop(flags);
    }

    void Audio::Play(UINT flags)
    {
        pSourceVoice->Start(flags);
    }

}