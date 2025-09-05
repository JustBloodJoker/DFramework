#include <Entity/RenderObject/TSkybox.h>
#include <World/World.h>

TSkybox::TSkybox(std::string path) {
	m_sPath = path;
	m_sName = path;
}

void TSkybox::AfterCreation() {
	TRender::AfterCreation();

	AddComponent<SkyboxComponent>(m_sPath);
}
