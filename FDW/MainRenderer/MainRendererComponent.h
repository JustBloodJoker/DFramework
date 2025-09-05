#pragma once 
#include <pch.h>
#include <D3DFramework/Utilites/Serializer/ReflectionImpl.h>
#include <System/NRenderSystemNotifyType.h>

class MainRenderer;
class World;

class MainRendererComponent {
public:
	MainRendererComponent()=default;
	virtual ~MainRendererComponent() = default;

	MainRenderer* Owner();

public:
	virtual void ProcessNotify(NRenderSystemNotifyType type);
	World* GetWorld();

public:
	BEGIN_FIELD_REGISTRATION(MainRendererComponent)
	END_FIELD_REGISTRATION()

public:
	void SetAfterConstruction(MainRenderer* owner);
	virtual void AfterConstruction();
	virtual void BeforeDestruction();
	
protected:
	MainRenderer* m_pOwner = nullptr;

};
