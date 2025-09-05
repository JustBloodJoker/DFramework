#pragma once

#include <pch.h>
#include <Entity/Core/IComponent.h>
#include <Entity/Light/LightComponentData.h>

class LightComponent : public IComponent {
public:
	LightComponent() = default;
	virtual ~LightComponent() = default;


public:
	BEGIN_FIELD_REGISTRATION(LightComponent, IComponent)
		REGISTER_FIELD(m_xData)
	END_FIELD_REGISTRATION();

public:

	LightComponentData GetLightComponentData();
	void SetLightComponentData(LightComponentData newData);

	virtual void Init() override;
	virtual void Activate(bool a) override;
	virtual void Destroy() override;

protected:
	LightComponentData m_xData;

};