#pragma once

#include <pch.h>
#include <Entity/Core/IComponent.h>
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

    virtual void BeforeRenderTick(float dt) override;
    virtual void Destroy() override;

public:
    BEGIN_FIELD_REGISTRATION(AudioComponent, IComponent)
        REGISTER_FIELD(m_sPath);
        REGISTER_FIELD(m_bIsLoop);
    END_FIELD_REGISTRATION();

protected:
    std::string m_sPath;
    std::unique_ptr<FD3DW::Audio> m_pAudio;
    bool m_bIsLoop = false;
};