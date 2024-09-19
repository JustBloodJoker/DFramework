#pragma once
#include "Object.h"


namespace FD3DW
{
	class SimpleObject
		: public Object
	{

	public:

		SimpleObject() = default;
		virtual ~SimpleObject() = default;

		
	protected:

		std::unique_ptr<FD3DW::UploadBuffer<FD3DW::VertexFrameWork>> pVertexUploadBuffer;
		std::unique_ptr<FD3DW::UploadBuffer<std::uint16_t>> pIndexUploadBuffer;
	};


	class Cube
		: public SimpleObject
	{
	public:

		Cube(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate);
		virtual ~Cube() = default;

	protected:

	};


	class Rectangle
		: public SimpleObject
	{

	public:

		Rectangle(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate);
		virtual ~Rectangle() = default;


	protected:

	};

	class Point
		: public SimpleObject
	{
	public:

		Point(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool neverUpdate);
		virtual ~Point() = default;

	protected:

	};

}