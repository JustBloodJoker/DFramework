#pragma once

#include <Entity/Core/ComponentHolder.h>
#include <Component/Audio/AudioComponent.h>

class TAudio : public ComponentHolder {
public:
	TAudio() = default;
	TAudio(std::string path);
	virtual ~TAudio() = default;

public:
	void Play();
	void Stop();
	void Restart();

	void SetVolume(float volume);
	float GetVolume();

	bool IsLoop();
	void Loop(bool loop);

public:
    REFLECT_BODY(TAudio)
    BEGIN_REFLECT(TAudio, ComponentHolder)
        REFLECT_PROPERTY(m_sPath)
    END_REFLECT(TAudio)

public:
	virtual void AfterCreation() override;

protected:
	std::string m_sPath;

};