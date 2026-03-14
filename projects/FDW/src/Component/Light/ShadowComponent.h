#pragma once

#include <pch.h>
#include <Component/Core/IComponent.h>
#include <Component/Light/ShadowComponentData.h>

class ShadowComponent : public IComponent {

public:
	ShadowComponent();
	virtual ~ShadowComponent() = default;


public:
    REFLECT_BODY(ShadowComponent)
    BEGIN_REFLECT(ShadowComponent, IComponent)
        REFLECT_PROPERTY(m_xData)
    END_REFLECT(ShadowComponent)


public:

	ShadowComponentData GetShadowComponentData();
	void SetShadowComponentData(ShadowComponentData newData);

	virtual void Init() override;
	virtual void Activate(bool a) override;
	virtual void Destroy() override;

protected:
	ShadowComponentData m_xData;

};
