#include <Entity/RenderObject/VerticesConverter.h>


FD3DW::VertexFrameWork SceneVertexToVertex(FD3DW::SceneVertexFrameWork a) {
	FD3DW::VertexFrameWork ret;
	ret.Pos = a.Pos;
	ret.Normal = a.Normal;
	ret.Bitangent = a.Bitangent;
	ret.Tangent = a.Tangent;
	ret.TexCoord = a.TexCoord;
	return ret;
}
