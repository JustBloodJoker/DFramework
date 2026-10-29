#pragma once
#include <System/NRenderSystemNotifyType.h>

class OfflineRenderer;
class World;

class OfflineRendererComponent {
public:
    OfflineRendererComponent()=default;
    virtual ~OfflineRendererComponent() = default;

public:
    virtual void ProcessNotify(NRenderSystemNotifyType type);
    World* GetWorld();

public:
	void SetAfterConstruction(OfflineRenderer* owner);
	virtual void AfterConstruction();

protected:
	OfflineRenderer* m_pOwner = nullptr;

};
