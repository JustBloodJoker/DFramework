
#include "../pch.h"
#include "Audio.h"

namespace FD3DW
{


    Audio::Audio(IXAudio2SourceVoice* source, UINT32 bufferSize, const BYTE* pAudioData)
    {
        m_uBufferSize = bufferSize;
        m_pAudioData = pAudioData;
        m_pSourceVoice = source;

        ReloadBufferData();
    }

    Audio::~Audio()
    {
        Stop();
        m_pSourceVoice->DestroyVoice();
        delete[] m_pAudioData;
    }

    float Audio::GetVolume() const
    {
        float ret;
        m_pSourceVoice->GetVolume(&ret);
        return ret;
    }

    void Audio::SetVolume(float volume)
    {
        m_pSourceVoice->SetVolume(volume);
    }

    void Audio::Stop(UINT flags)
    {
        m_pSourceVoice->Stop(flags);
    }

    bool Audio::IsEnded()
    {
        XAUDIO2_VOICE_STATE state = {};
        m_pSourceVoice->GetState(&state);
        return state.BuffersQueued == 0;
    }

    bool Audio::IsLoop()
    {
        return m_bIsLoop;
    }

    void Audio::Loop(bool loop, bool force)
    {
        m_bIsLoop = loop;

        if (force) {
            Restart();
        }
    }

    void Audio::ReloadBufferData()
    {
        XAUDIO2_BUFFER buffer = { 0 };
        buffer.AudioBytes = m_uBufferSize;
        buffer.pAudioData = m_pAudioData;
        buffer.Flags = XAUDIO2_END_OF_STREAM;
        
        if (m_bIsLoop) {
            buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
        }

        HRESULT_ASSERT(m_pSourceVoice->SubmitSourceBuffer(&buffer), "Submit buffer error");
    }

    void Audio::Restart(UINT playFlags, UINT endFlags)
    {
        Stop(endFlags);
        m_pSourceVoice->FlushSourceBuffers();
        ReloadBufferData();
        Play(playFlags);
    }

    void Audio::Play(UINT flags)
    {
        m_pSourceVoice->Start(flags);
    }

}