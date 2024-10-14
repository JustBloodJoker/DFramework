#include "../pch.h"
#include "Shader.h"


namespace FD3DW
{


	void Shader::GenerateBytecode(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint, const std::string& target, wrl::ComPtr<ID3DBlob>& result)
	{
		wrl::ComPtr<ID3DBlob> errors;

		UINT compileFlags = 0;
#if defined _DEBUG
		compileFlags = D3DCOMPILE_DEBUG |
			D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		HRESULT hr = S_OK;
		
		if (result)
			result->Release();

		hr = D3DCompileFromFile(filename.c_str(), defines,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entrypoint.c_str(), target.c_str(), compileFlags,
			0, result.GetAddressOf(), errors.GetAddressOf());
		
		if (errors != nullptr)
			OutputDebugStringA((char*)errors->GetBufferPointer());
		
		HRESULT_ASSERT(hr, "SHADER COMPILE ERROR");	
		
		CONSOLE_MESSAGE_NO_PREF(std::string("SHADER " + std::string(filename.begin(), filename.end()) +" ENTRY POINT: " + entrypoint + " success compiled"));
	}

	void Shader::LoadBytecode(const std::wstring& filename, wrl::ComPtr<ID3DBlob>& result)
	{
		std::fstream file(filename, std::ios::binary | std::ios::in);

		SAFE_ASSERT(file.is_open(), "file with shader bytecode doesn't open");

		file.seekg(0, std::ios_base::end);
		std::fstream::pos_type size = static_cast<int>(file.tellg());
		file.seekg(0, std::ios_base::beg);

		HRESULT_ASSERT(D3DCreateBlob(size, result.GetAddressOf()), "Create blob error");

		file.read((char*)result->GetBufferPointer(), size);

		CONSOLE_MESSAGE_NO_PREF(std::string("BYTECODE FILE " + std::string(filename.begin(), filename.end()) + " success gave data"));
	}

	void Shader::SaveBytecode(const std::wstring& filename, wrl::ComPtr<ID3DBlob>& inByteCode)
	{
		std::fstream file(filename, std::ios::binary | std::ios::out);

		SAFE_ASSERT(file.is_open(), "file with shader bytecode doesn't open");

		file.write((char*)inByteCode->GetBufferPointer(), inByteCode->GetBufferSize());

		CONSOLE_MESSAGE_NO_PREF(std::string("BYTECODE FILE " + std::string(filename.begin(), filename.end()) + " created! "));

		file.close();
	}


}