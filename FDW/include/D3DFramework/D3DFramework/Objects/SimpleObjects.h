#pragma once
#include "Object.h"

namespace FD3DW
{
	template<typename TVertex>
	class SimpleObject : public Object
	{
	public:
		using GeneratorFunc = std::function<void(std::vector<TVertex>&, std::vector<std::uint32_t>&)>;

		SimpleObject(ID3D12Device* pDevice,ID3D12GraphicsCommandList* pCommandList,GeneratorFunc generator)
		{

			generator(m_vVertices, m_vIndices);

			m_vObjectParameters.emplace_back(ObjectDesc{
				UINT(m_vVertices.size()),
				0u,
				UINT(m_vIndices.size()),
				0u,
				0u,
				ComputeMin(m_vVertices),
				ComputeMax(m_vVertices)
				});
		}

		virtual ~SimpleObject() = default;

		dx::XMFLOAT3 ComputeMin(const std::vector<TVertex>& vertices) {
			dx::XMFLOAT3 minPt(FLT_MAX, FLT_MAX, FLT_MAX);
			for (const auto& v : vertices) {
				minPt.x = std::min(minPt.x, v.Pos.x);
				minPt.y = std::min(minPt.y, v.Pos.y);
				minPt.z = std::min(minPt.z, v.Pos.z);
			}
			return minPt;
		}

		dx::XMFLOAT3 ComputeMax(const std::vector<TVertex>& vertices) {
			dx::XMFLOAT3 maxPt(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			for (const auto& v : vertices) {
				maxPt.x = std::max(maxPt.x, v.Pos.x);
				maxPt.y = std::max(maxPt.y, v.Pos.y);
				maxPt.z = std::max(maxPt.z, v.Pos.z);
			}
			return maxPt;
		}

		std::vector<TVertex> GetVertices() {
			return m_vVertices;
		}

		std::vector<std::uint32_t> GetIndices() {
			return m_vIndices;
		}

	protected:
		std::vector<TVertex> m_vVertices;
		std::vector<std::uint32_t> m_vIndices;

	};

	void GenerateCube(std::vector<VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
	void GenerateRectangle(std::vector<VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
	void GeneratePoint(std::vector<VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);

	//////////////////////////////////////
	////			DEFAULT TYPES
	class Cube : public SimpleObject<VertexFrameWork>
	{
	public:
		Cube(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
			: SimpleObject(device, cmdList, GenerateCube)
		{
		}
	};

	class Rectangle : public SimpleObject<VertexFrameWork>
	{
	public:
		Rectangle(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
			: SimpleObject(device, cmdList, GenerateRectangle)
		{
		}
	};

	class Point : public SimpleObject<VertexFrameWork>
	{
	public:
		Point(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
			: SimpleObject(device, cmdList, GeneratePoint)
		{
		}
	};

}