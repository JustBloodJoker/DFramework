#pragma once

#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>

#include <memory>

class ShadowAtlasRenderer;

class ShadowMetadataProvider {

	friend class ShadowAtlasRenderer;

protected:
	std::unique_ptr<FD3DW::StructuredBuffer> m_pLightAtlasMetaBuffer;

};
