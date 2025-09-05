#pragma once

#include <pch.h>
#include <Entity/Core/IComponent.h>
#include <Entity/Light/ShadowComponentData.h>

class ShadowComponent : public IComponent {

public:
	ShadowComponent() = default;
	virtual ~ShadowComponent() = default;


public:
	BEGIN_FIELD_REGISTRATION(ShadowComponent, IComponent)
		REGISTER_FIELD(m_xData)
	END_FIELD_REGISTRATION();


public:

	ShadowComponentData GetShadowComponentData();
	void SetShadowComponentData(ShadowComponentData newData);

	virtual void Init() override;
	virtual void Activate(bool a) override;
	virtual void Destroy() override;

protected:
	ShadowComponentData m_xData;

};
