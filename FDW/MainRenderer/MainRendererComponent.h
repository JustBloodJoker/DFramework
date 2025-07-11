#pragma once 
#include <pch.h>

class MainRenderer;

class MainRendererComponent {
public:
	MainRendererComponent(MainRenderer* owner);
	MainRendererComponent() = delete;
	virtual ~MainRendererComponent() = default;

	MainRenderer* Owner();

public:
	virtual void AfterConstruction();
	virtual void BeforeDestruction();
	
protected:
	MainRenderer* m_pOwner = nullptr;

};
