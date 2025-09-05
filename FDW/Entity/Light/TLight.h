#pragma once

#include <pch.h>
#include <Entity/Core/ComponentHolder.h>
#include <Entity/Light/LightComponent.h>
#include <Entity/Light/ShadowComponent.h>

#define IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(aaa, lors, type)    void SetLight##aaa##(type i) {										\
			if(!m_p##lors##Component) return;																						\
			auto data = m_p##lors##Component->Get##lors##ComponentData();															\
			data.aaa = i;																											\
			data.LightType = int( LightType() );																					\
			m_p##lors##Component->SetLightComponentData(data);																		\
		}																															\
																type GetLight##aaa##() const {										\
			return m_p##lors##Component ? m_p##lors##Component->Get##lors##ComponentData().aaa : type();							\
		}




class TLight : public ComponentHolder {
public:
	TLight()=default;
	virtual ~TLight() = default;


public:
	BEGIN_FIELD_REGISTRATION(TLight, ComponentHolder)
		REGISTER_FIELD(m_pLightComponent);
		REGISTER_FIELD(m_pShadowComponent);
	END_FIELD_REGISTRATION();


public:

	virtual void AfterCreation() override;
	virtual void OnComponentRemoved(IComponent* comp) override;

	virtual NLightComponentTypes LightType() = 0;

	bool IsHaveShadowComponent();
	bool IsHaveLightComponent();

	void ActivateLightComponent(bool b);
	bool IsActiveLightComponent();

	void ActivateShadowComponent(bool b);
	bool IsActiveShadowComponent();



	LightComponentData GetLightData() const;
	ShadowComponentData GetShadowData() const;

	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(Intensity, Light, float);
	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(Color, Light, dx::XMFLOAT3);


protected:
	LightComponent* m_pLightComponent = nullptr;
	ShadowComponent* m_pShadowComponent = nullptr;
};
