#pragma once

#include <pch.h>
#include <Component/Core/IComponent.h>
#include <Component/Light/LightComponentData.h>

class LightComponent : public IComponent {
public:
	LightComponent();
	virtual ~LightComponent() = default;


public:
    REFLECT_BODY(LightComponent)
    BEGIN_REFLECT(LightComponent, IComponent)
        REFLECT_PROPERTY(m_xData)
    END_REFLECT(LightComponent)

public:

	LightComponentData GetLightComponentData();
	void SetLightComponentData(LightComponentData newData);

	virtual void Init() override;
	virtual void Activate(bool a) override;
	virtual void Destroy() override;

protected:
	LightComponentData m_xData;

};