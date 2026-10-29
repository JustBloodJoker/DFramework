#pragma once

#include <Component/Core/IComponent.h>
#include <Component/Light/LightComponentData.h>
#include <Component/Light/LightRuntimeIdentity.h>

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
	std::uint64_t GetRuntimeLightIdentity() const;
	void SetLightComponentData(LightComponentData newData);
    void SetLightComponentDataSilent(const LightComponentData& newData);

	virtual void Init() override;
	virtual void Activate(bool a) override;
	virtual void Destroy() override;

protected:
	static LightComponentData NormalizeData(LightComponentData data);

protected:
	LightComponentData m_xData;
	LightRuntimeIdentity m_xRuntimeIdentity;

};
