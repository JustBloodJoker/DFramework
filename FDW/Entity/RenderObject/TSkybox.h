#pragma once

#include <pch.h>
#include <Entity/RenderObject/TRender.h>
#include <Component/RenderObject/SkyboxComponent.h>

class TSkybox : public TRender {
public:
	TSkybox() = default;
	TSkybox(std::string path);
	virtual ~TSkybox() = default;

public:
	BEGIN_FIELD_REGISTRATION(TSkybox, TRender)
		REGISTER_FIELD(m_sPath)
	END_FIELD_REGISTRATION();

public:
	virtual void AfterCreation() override;

protected:
	std::string m_sPath;
};