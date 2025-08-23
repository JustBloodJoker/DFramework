#pragma once
#include "../pch.h"



namespace FD3DW
{

	template <class T>
	constexpr auto& keep(T&& x) noexcept {
		return x;
	}


	inline DirectX::XMVECTOR ConvertFromAIVector3D(const aiVector3D& vec) noexcept
	{
		return DirectX::XMVECTOR({ vec.x, vec.y, vec.z, 1.0f });
	}

	inline DirectX::XMVECTOR ConvertFromAIQuaternion(const aiQuaternion& vec) noexcept
	{
		return DirectX::XMVECTOR({ vec.x, vec.y, vec.z, vec.w });
	}

	inline DirectX::XMMATRIX ConvertFromAIMatrix3X3(const aiMatrix3x3& matrix) noexcept
	{
		DirectX::XMMATRIX outputMatrix;
		outputMatrix.r[0] = DirectX::XMVectorSet(matrix.a1, matrix.a2, matrix.a3, 0.0f);
		outputMatrix.r[1] = DirectX::XMVectorSet(matrix.b1, matrix.b2, matrix.b3, 0.0f);
		outputMatrix.r[2] = DirectX::XMVectorSet(matrix.c1, matrix.c2, matrix.c3, 0.0f);
		outputMatrix.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		return outputMatrix;
	}

	inline DirectX::XMMATRIX ConvertFromAIMatrix4x4(const aiMatrix4x4& aiMat) {
		
		dx::XMMATRIX matrix;

		for (int y = 0; y < 4; y++)
		{
			for (int x = 0; x < 4; x++)
			{
				matrix(x,y) = aiMat[y][x];
			}
		}
		return matrix;
		
	}

	inline aiMatrix4x4 XMMatrixToAiMatrix4x4(const DirectX::XMMATRIX& dxMat) {
		auto matrix = dx::XMMatrixTranspose(dxMat);
		aiMatrix4x4 aiMat;
		for (int i = 0; i < 4; ++i) {
			aiMat[i][0] = matrix.r[i].vector4_f32[0];
			aiMat[i][1] = matrix.r[i].vector4_f32[1];
			aiMat[i][2] = matrix.r[i].vector4_f32[2];
			aiMat[i][3] = matrix.r[i].vector4_f32[3];
		}
		return aiMat;
	}


	inline UINT AlignForUavCounter(UINT bufferSize)
	{
		const UINT alignment = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
		return (bufferSize + (alignment - 1)) & ~(alignment - 1);
	}

	inline D3D12_SRV_DIMENSION ConvertDSVDimensionToSRVDimension(D3D12_DSV_DIMENSION dimension)
	{
		switch (dimension)
		{
		case D3D12_DSV_DIMENSION_TEXTURE1D:
			return D3D12_SRV_DIMENSION_TEXTURE1D;
		case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
			return D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
		case D3D12_DSV_DIMENSION_TEXTURE2D:
			return D3D12_SRV_DIMENSION_TEXTURE2D;
		case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
			return D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		case D3D12_DSV_DIMENSION_TEXTURE2DMS:
			return D3D12_SRV_DIMENSION_TEXTURE2DMS;
		case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
			return D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
		default:
			return D3D12_SRV_DIMENSION_UNKNOWN;
		}
	}

	inline unsigned GetChannelsCount(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_B4G4R4A4_UNORM:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:

			return 4;

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_B5G6R5_UNORM:

			return 3;

		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
			return 2;

		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
		case DXGI_FORMAT_R1_UNORM:
			return 1;
		
		case DXGI_FORMAT_UNKNOWN:
		default:
			return 0;
		}
	}
	inline UINT GetFormatSizeInBytes(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 16;

		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 12;

		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			return 8;

		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			return 4;

		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
			return 2;

		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
		case DXGI_FORMAT_UNKNOWN:
			return 1;

		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			return 8;

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 16;

		default:
			return 0;
		}
	}

	inline std::array<std::array<FLOAT, 4>, 3> XMMatrixToDXRMatrix(dx::XMMATRIX matrix)
	{
		std::array<std::array<FLOAT, 4>, 3> dxrMatrix;
		dx::XMMATRIX m = dx::XMMatrixTranspose(matrix);
		for (int row = 0; row < 3; ++row)
		{
			dxrMatrix[row][0] = m.r[row].vector4_f32[0];
			dxrMatrix[row][1] = m.r[row].vector4_f32[1];
			dxrMatrix[row][2] = m.r[row].vector4_f32[2];
			dxrMatrix[row][3] = m.r[row].vector4_f32[3];
		}
		return dxrMatrix;
	}

	inline void XMMATRIXSetToDXRMatrix(FLOAT(&out)[3][4], dx::XMMATRIX m) {
		dx::XMMATRIX t = dx::XMMatrixTranspose(m);
		for (int row = 0; row < 3; ++row)
		{
			out[row][0] = t.r[row].vector4_f32[0];
			out[row][1] = t.r[row].vector4_f32[1];
			out[row][2] = t.r[row].vector4_f32[2];
			out[row][3] = t.r[row].vector4_f32[3];
		}
	}



	inline std::string WStringToString(const std::wstring& wstr) {
		if (wstr.empty())
		{
			return std::string();
		}

		int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),static_cast<int>(wstr.size()),nullptr, 0, nullptr, nullptr);
		if (size_needed <= 0) {
			return std::string();
		}

		std::string result(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),static_cast<int>(wstr.size()),&result[0], size_needed, nullptr, nullptr);
		return result;
	}
	inline std::wstring StringToWString(const std::string& str) {
		if (str.empty()) {
			return std::wstring();
		}

		int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
		if (size_needed <= 0) {
			return std::wstring();
		}

		std::wstring result(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &result[0], size_needed);
		return result;
	}

}


