#pragma once
#include "Object.h"


namespace FDW
{
	class SimpleObject
		: public Object
	{

	public:

		SimpleObject() = default;
		virtual ~SimpleObject() = default;

		
	protected:

		std::vector<FDW::VertexFrameWork> vertices;
		std::vector<std::uint16_t> indices;

		void DeleteParameterVectors();

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