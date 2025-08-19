#pragma once

#include "../../pch.h"

namespace FD3DW {

	class ExecutionHandle {
	public:
		ExecutionHandle() = default;
		virtual ~ExecutionHandle() = default;

		void Bind(ID3D12Fence* fence, UINT64 val);
		void WaitForExecute();
		bool IsDone();

	public:
		ID3D12Fence* m_pFence = nullptr;
		UINT64 m_uValue = -1;
	};


}

