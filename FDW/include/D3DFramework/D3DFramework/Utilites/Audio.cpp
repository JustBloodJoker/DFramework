
#include "../pch.h"
#include "Audio.h"

namespace FD3DW
{


	Audio::Audio(IXAudio2SourceVoice* source)
	{
        this->m_pSourceVoice = source;
	}

    void Audio::SetVolume(float volume)
    {
        m_pSourceVoice->SetVolume(volume);
    }

    void Audio::Stop(UINT flags)
    {
        m_pSourceVoice->Stop(flags);
    }

    void Audio::Play(UINT flags)
    {
        m_pSourceVoice->Start(flags);
    }

}