#include "../pch.h"
#include "SimpleObjects.h"

namespace FD3DW {

	void GenerateCube(std::vector<VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices) {
		vertices = {
			{{-1, -1, -1}}, {{-1, +1, -1}}, {{+1, +1, -1}}, {{+1, -1, -1}},
			{{-1, -1, +1}}, {{-1, +1, +1}}, {{+1, +1, +1}}, {{+1, -1, +1}},
		};

		indices = {
			0,1,2, 0,2,3,
			4,6,5, 4,7,6,
			4,5,1, 4,1,0,
			3,2,6, 3,6,7,
			1,5,6, 1,6,2,
			4,0,3, 4,3,7
		};
	}

	void GenerateRectangle(std::vector<VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices)
	{
		vertices = {
			{ { -1.0f, -1.0f, 0.0f }, {}, { 0.0f, 1.0f }, {}, {} },
			{ {  1.0f, -1.0f, 0.0f }, {}, { 1.0f, 1.0f }, {}, {} },
			{ { -1.0f,  1.0f, 0.0f }, {}, { 0.0f, 0.0f }, {}, {} },
			{ {  1.0f,  1.0f, 0.0f }, {}, { 1.0f, 0.0f }, {}, {} },
		};

		indices = { 0, 1, 2, 1, 2, 3 };
	}

	void GeneratePoint(std::vector<VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices)
	{
		vertices = { FD3DW::VertexFrameWork{{0, 0, 0}} };
		indices = { 0 };
	}



}
