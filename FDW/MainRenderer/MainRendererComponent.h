#pragma once 
#include <pch.h>
#include <D3DFramework/Utilites/Serializer/ReflectionImpl.h>

class MainRenderer;

class MainRendererComponent {
public:
	MainRendererComponent()=default;
	virtual ~MainRendererComponent() = default;

	MainRenderer* Owner();

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
