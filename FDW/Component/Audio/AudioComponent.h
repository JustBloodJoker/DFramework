#pragma once

#include <pch.h>
#include <Component/Core/IComponent.h>
#include <D3DFramework/Utilites/AudioManager.h>

class AudioComponent : public IComponent {
public:
    AudioComponent() = default;
    AudioComponent(const std::string& path);
    virtual ~AudioComponent() = default;

public:
    virtual void Init() override;

public:
    void Play();
    void Stop();
    void Restart();

    void SetVolume(float volume);
    float GetVolume() const;

    bool IsLoop() const;
    void Loop(bool loop);

    virtual void AudioTick();
    virtual void Activate(bool a) override;
    virtual void Destroy() override;

public:
    REFLECT_BODY(AudioComponent)
    BEGIN_REFLECT(AudioComponent, IComponent)
        REFLECT_PROPERTY(m_sPath)
        REFLECT_PROPERTY(m_bIsLoop)
    END_REFLECT(AudioComponent)

protected:
    std::string m_sPath;
    std::unique_ptr<FD3DW::Audio> m_pAudio;
    bool m_bIsLoop = false;
};