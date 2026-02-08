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
    REFLECT_BODY(TSkybox)
    BEGIN_REFLECT(TSkybox, TRender)
        REFLECT_PROPERTY(m_sPath)
    END_REFLECT(TSkybox)

public:
	virtual void AfterCreation() override;

protected:
	std::string m_sPath;
};