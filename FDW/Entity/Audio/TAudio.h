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
	BEGIN_FIELD_REGISTRATION(TAudio, ComponentHolder)
		REGISTER_FIELD(m_sPath)
	END_FIELD_REGISTRATION()

public:
	virtual void AfterCreation() override;

protected:
	std::string m_sPath;

};