#pragma once
#include "../pch.h"


namespace FDW
{


	class RootSingature
	{

	public:

		RootSingature() = delete;
		~RootSingature() = default;;

		RootSingature(UINT numParameters);
		RootSingature(CD3DX12_ROOT_PARAMETER* slotRootParameters, UINT numParameters);


		void CreateRootSignature(ID3D12Device* pDevice, const D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, const UINT samplersCount = 0, D3D12_STATIC_SAMPLER_DESC* = nullptr, const UINT node = 0);
		void AddRootParameter(const CD3DX12_ROOT_PARAMETER& rootParameter);

		ID3D12RootSignature* GetRootSignature() const;

	private:

		
		std::vector<CD3DX12_ROOT_PARAMETER> slotRootParameters;
		wrl::ComPtr<ID3D12RootSignature> pRootSignature;

	};


}