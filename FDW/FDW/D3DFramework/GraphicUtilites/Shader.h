#pragma once
#include "../pch.h"


namespace FD3DW
{

	class Shader
	{


	public:

		static void GenerateBytecode(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint, const std::string& target, wrl::ComPtr<ID3DBlob>& result);
		static void LoadBytecode(const std::wstring& filename, wrl::ComPtr<ID3DBlob>& result);
		static void SaveBytecode(const std::wstring& filename, wrl::ComPtr<ID3DBlob>& inByteCode);

	private:


	};

}