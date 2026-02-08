#pragma once 
#include <pch.h>
#include <WinWindow/Utils/Reflection/Reflection.h>
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
    REFLECT_BODY(MainRendererComponent)
    BEGIN_REFLECT(MainRendererComponent)
    END_REFLECT(MainRendererComponent)

public:
	void SetAfterConstruction(MainRenderer* owner);
	virtual void AfterConstruction();
	virtual void BeforeDestruction();
	
protected:
	MainRenderer* m_pOwner = nullptr;

};
