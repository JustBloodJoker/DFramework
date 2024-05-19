#pragma once
#include "../pch.h"
#include "Object.h"


namespace FDW
{


	class Scene
		: public Object
	{

	public:

		Scene(std::string path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate);
		virtual ~Scene();

		

	private:

		

		std::vector<FDW::VertexFrameWork> vertices;
		std::vector<std::uint16_t> indices;
		void ParseScene(std::string& path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);

	};


}